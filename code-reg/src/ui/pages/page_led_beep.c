#include "page_led_beep.h"
#include "beep/task_beep.h"
#include "components/ui_spinebox.h"
#include "components/ui_slider.h"
#include "components/ui_page_base.h"
#include "components/ui_form.h"
#include "lvgl.h"
#include "task.h"
#include <stdio.h>

#define LED_BEEP_BUTTON_GAP 12

/* 输出页面初始化阶段的采样结果；调用前所有时间点已经完成采样。 */
static void led_beep_print_timing(TickType_t start_tick,
                                  TickType_t page_base_tick,
                                  TickType_t title_tick,
                                  TickType_t loop_mode_tick,
                                  TickType_t duration_tick,
                                  TickType_t buttons_tick,
                                  TickType_t end_tick) {
  printf("[led_beep] page_base=%lu title=%lu loop_mode=%lu "
         "duration=%lu buttons=%lu finalize=%lu total=%lu ms\r\n",
         (unsigned long)((page_base_tick - start_tick) * portTICK_PERIOD_MS),
         (unsigned long)((title_tick - page_base_tick) * portTICK_PERIOD_MS),
         (unsigned long)((loop_mode_tick - title_tick) * portTICK_PERIOD_MS),
         (unsigned long)((duration_tick - loop_mode_tick) * portTICK_PERIOD_MS),
         (unsigned long)((buttons_tick - duration_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - buttons_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/* 页面中每组参数控件的统一对象引用。
 * row         : 当前参数所在的容器，用于整体显示或隐藏。
 * slider      : 用户拖动的数值调节控件。
 * value_label : 显示滑块当前数值和单位。
 * unit        : 数值后缀，例如 Hz 或 ms。
 */
typedef struct {
  lv_obj_t *row;
  lv_obj_t *slider;
  lv_obj_t *spinebox;
  lv_obj_t *value_label;
  ui_spinebox_t spinebox_component;
  ui_slider_t slider_component;
  ui_form_row_t form_row;
  bool use_spinebox;
  const char *unit;
} led_beep_value_control_t;

/* 页面级控件引用，供各个 LVGL 事件回调访问。 */
static lv_obj_t *g_loop_checkbox;
static ui_form_t g_form;
static led_beep_value_control_t g_frequency;
static led_beep_value_control_t g_play_time;
static led_beep_value_control_t g_stop_time;
static led_beep_value_control_t g_single_time;

static void led_beep_create_loop_controls(void);

/* 将参数控件当前值同步到右侧数值标签。 */
static void led_beep_update_value_label(
    led_beep_value_control_t *control) {
  int32_t value = control->spinebox != NULL
                      ? ui_spinebox_get_value(&control->spinebox_component)
                      : lv_slider_get_value(control->slider);

  lv_label_set_text_fmt(control->value_label, "%ld %s", (long)value,
                        control->unit);
}

/* 获取参数控件当前值。频率使用数字输入框，其余参数使用滑块。 */
static int32_t led_beep_get_control_value(
    const led_beep_value_control_t *control) {
  return control->spinebox != NULL
             ? ui_spinebox_get_value(&control->spinebox_component)
             : lv_slider_get_value(control->slider);
}

/* 频率变化时，按照周期关系同步播放时长和停止时长。
 * 频率单位为 Hz，周期单位为 ms，计算关系为：
 *
 *     period_ms = 1000 / frequency_hz
 *
 * 播放时长和停止时长都设置为一个周期；当计算结果小于滑块
 * 允许的最小值 10 ms 时，使用 10 ms，避免滑块出现无效值。
 */
static void led_beep_sync_period_controls(void) {
  int32_t frequency = led_beep_get_control_value(&g_frequency);
  int32_t period_ms;

  if (frequency <= 0)
    return;

  period_ms = 1000 / frequency;
  if (period_ms < 10)
    period_ms = 10;

  lv_slider_set_value(g_play_time.slider, period_ms, LV_ANIM_OFF);
  lv_slider_set_value(g_stop_time.slider, period_ms, LV_ANIM_OFF);
  led_beep_update_value_label(&g_play_time);
  led_beep_update_value_label(&g_stop_time);
}

/* 滑块和频率数字输入框共用的数值变化事件。 */
static void led_beep_value_event_cb(lv_event_t *event) {
  led_beep_value_control_t *control =
      (led_beep_value_control_t *)lv_event_get_user_data(event);

  if (control->value_label != NULL)
    led_beep_update_value_label(control);
  if (control == &g_frequency)
    led_beep_sync_period_controls();
}

/* 保留原回调名称，兼容已有滑块事件注册代码。 */
static void led_beep_slider_event_cb(lv_event_t *event) {
  led_beep_value_event_cb(event);
}

/* 循环模式复选框的值变化事件。
 * 循环模式显示频率、播放时长和停止时长；
 * 非循环模式只显示一次播放的持续时间。
 * 参数控件只做隐藏或显示，不销毁控件，以保留用户已经设置的数值。
 */
static void led_beep_loop_mode_event_cb(lv_event_t *event) {
  bool loop_mode =
      lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED);

  if (loop_mode) {
    led_beep_create_loop_controls();
    lv_obj_clear_flag(g_frequency.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_play_time.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_stop_time.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_single_time.row, LV_OBJ_FLAG_HIDDEN);
  } else {
    if (g_frequency.row != NULL) {
      lv_obj_add_flag(g_frequency.row, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(g_play_time.row, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(g_stop_time.row, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(g_single_time.row, LV_OBJ_FLAG_HIDDEN);
  }
}

/* 播放按钮点击事件。
 * 非循环模式调用单次播放接口；
 * 循环模式调用播放/停止交替的循环播放接口。
 */
static void led_beep_play_event_cb(lv_event_t *event) {
  bool loop_mode;

  (void)event;

  loop_mode = lv_obj_has_state(g_loop_checkbox, LV_STATE_CHECKED);
  if (loop_mode) {
    /* 当前蜂鸣器驱动仍是 GPIO 开关，频率参数暂未传给底层驱动。 */
    playBeepLoop((uint32_t)led_beep_get_control_value(&g_play_time),
                 (uint32_t)led_beep_get_control_value(&g_stop_time));
  } else {
    playBeep((uint32_t)led_beep_get_control_value(&g_single_time));
  }
}

/* 停止按钮点击事件。
 * 无论当前处于单次播放还是循环播放，都立即关闭蜂鸣器，
 * 并取消已经安排的下一次定时切换。
 */
static void led_beep_stop_event_cb(lv_event_t *event) {
  (void)event;
  stopBeep();
}

/* 创建一个参数控制行。
 * 每行包含参数名称和数值调节控件；滑块控件还显示当前数值标签。
 * control 保存控件引用，供数值变化事件读取当前值并更新标签。
 */
static void led_beep_create_value_control(ui_form_t *form,
                                           led_beep_value_control_t *control,
                                           const char *name,
                                           const char *unit,
                                           int32_t min,
                                           int32_t max,
                                           int32_t current,
                                           bool use_spinebox) {
  lv_obj_t *control_parent;

  control->slider = NULL;
  control->spinebox = NULL;
  control->value_label = NULL;
  control->unit = unit;
  control->use_spinebox = use_spinebox;
  ui_form_row_create(form, &control->form_row, name);
  control->row = control->form_row.container;
  control_parent = control->form_row.container;

  if (use_spinebox) {
    ui_spinebox_create(&control->spinebox_component, control_parent, min, max,
                      current, 1, 3);
    control->spinebox = control->spinebox_component.spinebox;
    ui_form_row_set_control(&control->form_row,
                            control->spinebox_component.container);
    ui_spinebox_add_event_cb(&control->spinebox_component,
                            led_beep_value_event_cb,
                            LV_EVENT_VALUE_CHANGED, control);
  } else {
    ui_slider_create(&control->slider_component, control_parent, min, max,
                     current, unit);
    control->slider = control->slider_component.slider;
    ui_form_row_set_control(&control->form_row,
                            control->slider_component.container);
    control->value_label = control->slider_component.value_label;
    ui_slider_add_event_cb(&control->slider_component,
                           led_beep_slider_event_cb,
                           LV_EVENT_VALUE_CHANGED, control);
  }
}

static void led_beep_create_loop_controls(void) {
  if (g_frequency.row != NULL)
    return;

  led_beep_create_value_control(&g_form, &g_frequency, "Frequency", "Hz", 1,
                                100, 10, true);
  led_beep_create_value_control(&g_form, &g_play_time, "Play time", "ms", 10,
                                5000, 100, false);
  led_beep_create_value_control(&g_form, &g_stop_time, "Stop time", "ms", 10,
                                5000, 100, false);
}

/* 初始化蜂鸣器控制页面。
 * 默认隐藏的循环模式参数在首次开启循环模式时创建。
 */
void view_led_beep_init(void *params) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t page_base_tick;
  TickType_t title_tick;
  TickType_t loop_mode_tick;
  TickType_t duration_tick;
  TickType_t buttons_tick;
  TickType_t end_tick;
  lv_obj_t *scr = lv_scr_act();
  lv_obj_t *title;
  lv_obj_t *play_button;
  lv_obj_t *stop_button;
  ui_form_row_t loop_mode_form_row;
  lv_obj_t *button_row;
  lv_obj_t *play_label;
  lv_obj_t *stop_label;

  (void)params;

  /* 页面基础布局统一处理尺寸、系统栏和 Dock 避让、边距及 Flex 间距。 */
  ui_page_base_init(scr);
  page_base_tick = xTaskGetTickCount();

  /* 页面标题。 */
  title = lv_label_create(scr);
  lv_label_set_text(title, "Buzzer Control");
  title_tick = xTaskGetTickCount();

  /* Form 统一管理页面中的参数行和控件布局。 */
  g_frequency.row = NULL;
  g_play_time.row = NULL;
  g_stop_time.row = NULL;
  ui_form_create(&g_form, scr);
  ui_form_row_create(&g_form, &loop_mode_form_row, "Loop mode");
  g_loop_checkbox = lv_switch_create(loop_mode_form_row.container);
  ui_form_row_set_control(&loop_mode_form_row, g_loop_checkbox);
  lv_obj_add_event_cb(g_loop_checkbox, led_beep_loop_mode_event_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);
  loop_mode_tick = xTaskGetTickCount();

  /* 默认只创建非循环模式参数。 */
  led_beep_create_value_control(&g_form, &g_single_time, "Duration", "ms", 10,
                                10000, 1000, false);
  duration_tick = xTaskGetTickCount();

  /* 创建播放和停止按钮所在的横向操作区。 */
  button_row = lv_obj_create(scr);
  lv_obj_set_width(button_row, LV_PCT(100));
  lv_obj_set_height(button_row, 60);
  lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(button_row, 0, 0);
  lv_obj_set_style_border_width(button_row, 0, 0);
  lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_column(button_row, LED_BEEP_BUTTON_GAP, 0);
  lv_obj_set_layout(button_row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  /* 播放按钮：读取当前页面参数并启动蜂鸣器。 */
  play_button = lv_btn_create(button_row);
  lv_obj_set_size(play_button, 120, 48);
  play_label = lv_label_create(play_button);
  lv_label_set_text(play_label, "Play");
  lv_obj_center(play_label);
  lv_obj_add_event_cb(play_button, led_beep_play_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /* 停止按钮：立即关闭蜂鸣器并取消后续循环定时。 */
  stop_button = lv_btn_create(button_row);
  lv_obj_set_size(stop_button, 120, 48);
  stop_label = lv_label_create(stop_button);
  lv_label_set_text(stop_label, "Stop");
  lv_obj_center(stop_label);
  lv_obj_add_event_cb(stop_button, led_beep_stop_event_cb, LV_EVENT_CLICKED,
                      NULL);
  buttons_tick = xTaskGetTickCount();
  end_tick = xTaskGetTickCount();

  led_beep_print_timing(start_tick, page_base_tick, title_tick,
                        loop_mode_tick, duration_tick, buttons_tick,
                        end_tick);
}
