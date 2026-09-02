#include "page_led_beep.h"
#include "beep/task_beep.h"
#include "components/ui_spinbox.h"
#include "lvgl.h"

#define LED_BEEP_CONTENT_WIDTH 360
#define LED_BEEP_CONTROL_WIDTH 300

/* 页面中每组参数控件的统一对象引用。
 * row         : 当前参数所在的容器，用于整体显示或隐藏。
 * slider      : 用户拖动的数值调节控件。
 * value_label : 显示滑块当前数值和单位。
 * unit        : 数值后缀，例如 Hz 或 ms。
 */
typedef struct {
  lv_obj_t *row;
  lv_obj_t *slider;
  lv_obj_t *spinbox;
  lv_obj_t *value_label;
  ui_spinbox_t spinbox_component;
  bool use_spinbox;
  const char *unit;
} led_beep_value_control_t;

/* 页面级控件引用，供各个 LVGL 事件回调访问。 */
static lv_obj_t *g_loop_checkbox;
static led_beep_value_control_t g_frequency;
static led_beep_value_control_t g_play_time;
static led_beep_value_control_t g_stop_time;
static led_beep_value_control_t g_single_time;

/* 将参数控件当前值同步到右侧数值标签。 */
static void led_beep_update_value_label(
    led_beep_value_control_t *control) {
  int32_t value = control->spinbox != NULL
                      ? lv_spinbox_get_value(control->spinbox)
                      : lv_slider_get_value(control->slider);

  lv_label_set_text_fmt(control->value_label, "%ld %s", (long)value,
                        control->unit);
}

/* 获取参数控件当前值。频率使用数字输入框，其余参数使用滑块。 */
static int32_t led_beep_get_control_value(
    const led_beep_value_control_t *control) {
  return control->spinbox != NULL
             ? lv_spinbox_get_value(control->spinbox)
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
    lv_obj_clear_flag(g_frequency.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_play_time.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_stop_time.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_single_time.row, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(g_frequency.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_play_time.row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_stop_time.row, LV_OBJ_FLAG_HIDDEN);
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
 * 每行包含参数名称、当前数值标签和底部滑块。
 * control 保存控件引用，供滑块事件回调读取当前值并更新标签。
 */
static void led_beep_create_value_control(lv_obj_t *parent,
                                           led_beep_value_control_t *control,
                                           const char *name,
                                           const char *unit,
                                           int32_t min,
                                           int32_t max,
                                           int32_t current,
                                           bool use_spinbox) {
  lv_obj_t *name_label;

  control->row = lv_obj_create(parent);
  control->slider = NULL;
  control->spinbox = NULL;
  control->value_label = lv_label_create(control->row);
  control->unit = unit;
  control->use_spinbox = use_spinbox;
  name_label = lv_label_create(control->row);

  /* 参数行高度根据控件类型调整，避免较高的数字输入框覆盖参数名称。 */
  lv_obj_set_width(control->row, LED_BEEP_CONTENT_WIDTH);
  lv_obj_set_height(control->row, use_spinbox ? 82 : 58);
  lv_obj_clear_flag(control->row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(control->row, 6, 0);
  lv_obj_set_style_border_width(control->row, 0, 0);
  lv_obj_set_style_bg_opa(control->row, LV_OPA_TRANSP, 0);

  /* 参数名称放在行的左上角。 */
  lv_label_set_text(name_label, name);
  lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);

  if (use_spinbox) {
    ui_spinbox_create(&control->spinbox_component, control->row, min, max,
                      current, 1, 3);
    control->spinbox = control->spinbox_component.spinbox;
    ui_spinbox_add_event_cb(&control->spinbox_component,
                            led_beep_value_event_cb,
                            LV_EVENT_VALUE_CHANGED, control);
    lv_obj_add_flag(control->value_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    /* 播放时长和停止时长继续使用滑块调节。 */
    control->slider = lv_slider_create(control->row);
    lv_slider_set_range(control->slider, min, max);
    lv_slider_set_value(control->slider, current, LV_ANIM_OFF);
    lv_obj_set_width(control->slider, LED_BEEP_CONTROL_WIDTH);
    lv_obj_align(control->slider, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_event_cb(control->slider, led_beep_slider_event_cb,
                        LV_EVENT_VALUE_CHANGED, control);
  }

  /* 数值标签放在行的右上角，并显示当前值及单位。 */
  if (!use_spinbox)
    lv_label_set_text_fmt(control->value_label, "%ld %s", (long)current,
                          unit);
  lv_obj_align(control->value_label, LV_ALIGN_TOP_RIGHT, 0, 0);

  /* 频率输入框和滑块都已经注册数值变化回调。 */
}

/* 初始化蜂鸣器控制页面。
 * 本函数只负责创建和配置 LVGL 控件，实际蜂鸣器输出由 task_beep
 * 模块负责，页面通过 playBeep() 或 playBeepLoop() 启动播放。
 */
void view_led_beep_init(void *params) {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_t *content;
  lv_obj_t *title;
  lv_obj_t *play_button;
  lv_obj_t *stop_button;
  lv_obj_t *button_row;
  lv_obj_t *loop_mode_row;
  lv_obj_t *loop_mode_label;
  lv_obj_t *play_label;
  lv_obj_t *stop_label;

  (void)params;

  /* 页面使用单列 Flex 布局，控件按照创建顺序从上到下排列。 */
  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_top(scr, 24, 0);
  lv_obj_set_style_pad_row(scr, 12, 0);

  /* 页面标题。 */
  title = lv_label_create(scr);
  lv_label_set_text(title, "Buzzer Control");

  /* 参数内容面板使用垂直 Flex 布局，参数行会自动从上到下排列。 */
  content = lv_obj_create(scr);
  lv_obj_set_width(content, LED_BEEP_CONTENT_WIDTH);
  lv_obj_set_height(content, LV_SIZE_CONTENT);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(content, 12, 0);
  lv_obj_set_style_pad_row(content, 12, 0);
  lv_obj_set_layout(content, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_START);

  /* 循环模式开关。默认关闭，因此页面初始为单次播放模式。 */
  loop_mode_row = lv_obj_create(content);
  lv_obj_set_width(loop_mode_row, LED_BEEP_CONTENT_WIDTH);
  lv_obj_set_height(loop_mode_row, 40);
  lv_obj_clear_flag(loop_mode_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(loop_mode_row, 0, 0);
  lv_obj_set_style_border_width(loop_mode_row, 0, 0);
  lv_obj_set_style_bg_opa(loop_mode_row, LV_OPA_TRANSP, 0);
  loop_mode_label = lv_label_create(loop_mode_row);
  lv_label_set_text(loop_mode_label, "Loop mode");
  lv_obj_align(loop_mode_label, LV_ALIGN_LEFT_MID, 0, 0);
  g_loop_checkbox = lv_switch_create(loop_mode_row);
  lv_obj_align(g_loop_checkbox, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(g_loop_checkbox, led_beep_loop_mode_event_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);

  /* 创建循环模式参数：频率、播放时长和停止时长。 */
  led_beep_create_value_control(content, &g_frequency, "Frequency", "Hz", 1,
                                100, 10, true);
  led_beep_create_value_control(content, &g_play_time, "Play time", "ms", 10,
                                5000, 100, false);
  led_beep_create_value_control(content, &g_stop_time, "Stop time", "ms", 10,
                                5000, 100, false);

  /* 创建非循环模式参数：一次播放的总持续时间。 */
  led_beep_create_value_control(content, &g_single_time, "Duration", "ms", 10,
                                10000, 1000, false);

  /* 默认隐藏循环模式参数，只显示单次播放的持续时间。 */
  lv_obj_add_flag(g_frequency.row, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_play_time.row, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_stop_time.row, LV_OBJ_FLAG_HIDDEN);

  /* 创建播放和停止按钮所在的横向容器。 */
  button_row = lv_obj_create(content);
  lv_obj_set_size(button_row, LED_BEEP_CONTENT_WIDTH, 60);
  lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(button_row, 0, 0);
  lv_obj_set_style_border_width(button_row, 0, 0);
  lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
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
}
