#include "page_beep.h"
#include "../components/ui_param.h"
#include "beep/task_beep.h"
#include "lvgl.h"
#include "task.h"
#include <stdio.h>

/* ==================== 布局常量 ====================
 * 页面内容区 432 x 636（480 x 684 减去四周 24 内边距），
 * 全部使用固定坐标，避免运行时布局计算开销。
 */
#define BEEP_PILL_WIDTH 204
#define BEEP_PILL_HEIGHT 64
#define BEEP_PILL_GAP 24
#define BEEP_PILL_Y 0

#define BEEP_CARD_X 0
#define BEEP_CARD_Y 88
#define BEEP_CARD_WIDTH 432
#define BEEP_CARD_HEIGHT 520

/* 参数行统一布局（相对 section 容器）：单行内名称标签在左、
 * [− 值 +] 步进组由 ui_param 组件摆放。
 * 单次模式的时长行与循环模式的频率、占空比行样式完全一致。 */
#define ROW_NAME_X 24
#define ROW_HEIGHT 48
#define ROW_TEXT_DY 12

/* 单次模式：时长参数行放在第一行位置。 */
#define SINGLE_DURATION_ROW_Y 28

/* 循环模式：频率、占空比两行。 */
#define LOOP_FREQ_ROW_Y 28
#define LOOP_DUTY_ROW_Y 88

/* 启停切换按钮位置（单一按钮，水平居中，相对 section 容器）。 */
#define ACTION_WIDTH 180
#define ACTION_HEIGHT 72
#define ACTION_X ((BEEP_CARD_WIDTH - ACTION_WIDTH) / 2)
#define ACTION_Y_SINGLE 360
#define ACTION_Y_LOOP 340

/* ==================== 颜色 ==================== */
#define BEEP_COLOR_BLUE 0x3A76F4
#define BEEP_COLOR_BLUE_PRESSED 0x2E5FCC
#define BEEP_COLOR_PILL_BG 0xFFFFFF
#define BEEP_COLOR_PILL_BORDER 0xC8C8C8
#define BEEP_COLOR_STOP_BG 0xE8E8E8
#define BEEP_COLOR_STOP_PRESSED 0xD5D5D5
#define BEEP_COLOR_TEXT_DARK 0x1A1A1A

/* ==================== 可调参数 ====================
 * 参数与步进控件复用 ui_param 基础组件。
 * 频率 / 占空比在循环播放运行中调整时立即重设蜂鸣周期。 */
static void beep_loop_param_changed(ui_param_t *param);

static ui_param_t g_duration = {100, 10, 10000, 10, " ms", NULL, NULL,
                                NULL, NULL};
static ui_param_t g_frequency = {50, 1, 100, 1, " Hz", NULL, NULL,
                                 NULL, beep_loop_param_changed};
static ui_param_t g_duty = {50, 5, 95, 1, "%", NULL, NULL,
                            NULL, beep_loop_param_changed};

/* 步进按钮上下文（静态分配，页面销毁时清零）。 */
static ui_step_btn_t g_single_minus = {NULL, -1, 0};
static ui_step_btn_t g_single_plus = {NULL, 1, 0};
static ui_step_btn_t g_freq_minus = {NULL, -1, 0};
static ui_step_btn_t g_freq_plus = {NULL, 1, 0};
static ui_step_btn_t g_duty_minus = {NULL, -1, 0};
static ui_step_btn_t g_duty_plus = {NULL, 1, 0};

/* 页面级控件引用。 */
static lv_obj_t *g_pill_single;
static lv_obj_t *g_pill_loop;
static lv_obj_t *g_section_single;
static lv_obj_t *g_section_loop;
static lv_obj_t *g_action_single;
static lv_obj_t *g_action_loop;
static bool g_loop_mode;
/* 蜂鸣器运行中：单一硬件通道，两个分区共用同一个启停状态。 */
static bool g_beep_running;
/* 单次播放完成定时器：到期后把按钮复位为 Start，提前停止时删除。 */
static lv_timer_t *g_single_timer;

/* ==================== 共用样式 ====================
 * 所有自定义样式只在首次创建页面时构建一次，页面控件直接引用。
 */
enum {
  STYLE_CARD = 0,
  STYLE_SECTION,
  STYLE_PILL,
  STYLE_START_BTN,
  STYLE_NAME_LABEL,
  STYLE_COUNT
};

static lv_style_t g_styles[STYLE_COUNT];
static bool g_styles_initialized;

static void beep_styles_init(void) {
  if (g_styles_initialized)
    return;

  /* 白色圆角卡片。 */
  lv_style_init(&g_styles[STYLE_CARD]);
  lv_style_set_bg_color(&g_styles[STYLE_CARD], lv_color_hex(0xFFFFFF));
  lv_style_set_bg_opa(&g_styles[STYLE_CARD], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_CARD], 16);
  lv_style_set_border_width(&g_styles[STYLE_CARD], 0);
  lv_style_set_pad_all(&g_styles[STYLE_CARD], 0);
  lv_style_set_text_color(&g_styles[STYLE_CARD],
                          lv_color_hex(BEEP_COLOR_TEXT_DARK));

  /* 透明分区容器，用于整体显示 / 隐藏一种模式的全部控件。 */
  lv_style_init(&g_styles[STYLE_SECTION]);
  lv_style_set_bg_opa(&g_styles[STYLE_SECTION], LV_OPA_TRANSP);
  lv_style_set_border_width(&g_styles[STYLE_SECTION], 0);
  lv_style_set_pad_all(&g_styles[STYLE_SECTION], 0);

  /* 模式切换胶囊按钮（未选中外观，选中态在切换时直接覆盖颜色）。 */
  lv_style_init(&g_styles[STYLE_PILL]);
  lv_style_set_bg_color(&g_styles[STYLE_PILL], lv_color_hex(BEEP_COLOR_PILL_BG));
  lv_style_set_bg_opa(&g_styles[STYLE_PILL], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_PILL], LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&g_styles[STYLE_PILL], 1);
  lv_style_set_border_color(&g_styles[STYLE_PILL],
                            lv_color_hex(BEEP_COLOR_PILL_BORDER));
  lv_style_set_pad_all(&g_styles[STYLE_PILL], 0);
  lv_style_set_text_color(&g_styles[STYLE_PILL],
                          lv_color_hex(BEEP_COLOR_TEXT_DARK));

  /* 启停切换按钮：默认（停止态）蓝色，运行态颜色由刷新函数覆盖。 */
  lv_style_init(&g_styles[STYLE_START_BTN]);
  lv_style_set_bg_color(&g_styles[STYLE_START_BTN],
                        lv_color_hex(BEEP_COLOR_BLUE));
  lv_style_set_bg_opa(&g_styles[STYLE_START_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_START_BTN], 12);
  lv_style_set_border_width(&g_styles[STYLE_START_BTN], 0);
  lv_style_set_pad_all(&g_styles[STYLE_START_BTN], 0);
  lv_style_set_text_color(&g_styles[STYLE_START_BTN], lv_color_white());

  /* 参数名称文字（数值样式由 ui_param 组件提供）。 */
  lv_style_init(&g_styles[STYLE_NAME_LABEL]);
  lv_style_set_text_color(&g_styles[STYLE_NAME_LABEL],
                          lv_color_hex(BEEP_COLOR_TEXT_DARK));

  g_styles_initialized = true;
}

/* ==================== 数值显示与步进 ====================
 * 步进机制与 [− 值 +] 步进组布局全部由 ui_param 基础组件提供，
 * 蜂鸣器页面仅负责名称标签与行的摆放。 */

/* 创建启停切换按钮（外观与文字由 beep_refresh_action 按运行状态刷新）。 */
static lv_obj_t *beep_create_action_btn(lv_obj_t *parent, lv_coord_t y) {
  lv_obj_t *button = lv_btn_create(parent);

  lv_obj_remove_style_all(button);
  lv_obj_add_style(button, &g_styles[STYLE_START_BTN], 0);
  lv_obj_set_size(button, ACTION_WIDTH, ACTION_HEIGHT);
  lv_obj_set_pos(button, ACTION_X, y);

  {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text_static(label, "Start");
    lv_obj_center(label);
  }

  return button;
}

/* ==================== 模式切换 ==================== */

/* 刷新两个胶囊按钮的选中态外观。 */
static void beep_refresh_pills(void) {
  lv_obj_t *active = g_loop_mode ? g_pill_loop : g_pill_single;
  lv_obj_t *inactive = g_loop_mode ? g_pill_single : g_pill_loop;

  lv_obj_set_style_bg_color(active, lv_color_hex(BEEP_COLOR_BLUE), 0);
  lv_obj_set_style_text_color(active, lv_color_white(), 0);
  lv_obj_set_style_border_width(active, 0, 0);

  lv_obj_set_style_bg_color(inactive, lv_color_hex(BEEP_COLOR_PILL_BG), 0);
  lv_obj_set_style_text_color(inactive,
                              lv_color_hex(BEEP_COLOR_TEXT_DARK), 0);
  lv_obj_set_style_border_width(inactive, 1, 0);
}

static void beep_set_mode(bool loop_mode) {
  if (g_loop_mode == loop_mode)
    return;

  g_loop_mode = loop_mode;

  if (loop_mode) {
    lv_obj_add_flag(g_section_single, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_section_loop, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(g_section_single, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_section_loop, LV_OBJ_FLAG_HIDDEN);
  }

  beep_refresh_pills();
}

static void beep_pill_single_event_cb(lv_event_t *event) {
  (void)event;
  beep_set_mode(false);
}

static void beep_pill_loop_event_cb(lv_event_t *event) {
  (void)event;
  beep_set_mode(true);
}

/* ==================== 启动 / 停止 ==================== */

/* 按频率与占空比折算播放 / 停止时长：
 * 频率参数暂未传给底层驱动（GPIO 开关），按周期换算。 */
static void beep_loop_timing(uint32_t *play_ms_out, uint32_t *stop_ms_out) {
  int32_t period_ms = 1000 / g_frequency.value;
  int32_t play_ms;
  int32_t stop_ms;

  if (period_ms < 10)
    period_ms = 10;
  if (period_ms > 5000)
    period_ms = 5000;

  play_ms = period_ms * g_duty.value / 100;
  stop_ms = period_ms - play_ms;
  if (play_ms < 10)
    play_ms = 10;
  if (stop_ms < 10)
    stop_ms = 10;

  *play_ms_out = (uint32_t)play_ms;
  *stop_ms_out = (uint32_t)stop_ms;
}

/* 循环播放运行中频率 / 占空比变化：立即按新周期重新开始播放。 */
static void beep_loop_param_changed(ui_param_t *param) {
  uint32_t play_ms;
  uint32_t stop_ms;

  (void)param;

  if (!g_beep_running || !g_loop_mode)
    return;

  beep_loop_timing(&play_ms, &stop_ms);
  playBeepLoop(play_ms, stop_ms);
}

/* 按当前模式把参数输出到蜂鸣器硬件。 */
static void beep_hw_start(void) {
  uint32_t play_ms;
  uint32_t stop_ms;

  if (g_loop_mode) {
    beep_loop_timing(&play_ms, &stop_ms);
    playBeepLoop(play_ms, stop_ms);
  } else {
    playBeep((uint32_t)g_duration.value);
  }
}

/* 刷新两个分区启停按钮的外观与文字（共用同一运行状态）。 */
static void beep_refresh_action(void) {
  lv_obj_t *buttons[2] = {g_action_single, g_action_loop};
  const char *text = g_beep_running ? "Stop" : "Start";
  uint32_t i;

  for (i = 0; i < 2; i++) {
    lv_obj_t *button = buttons[i];
    lv_obj_t *label;

    if (button == NULL)
      continue;

    if (g_beep_running) {
      lv_obj_set_style_bg_color(button, lv_color_hex(BEEP_COLOR_STOP_BG), 0);
      lv_obj_set_style_bg_color(button, lv_color_hex(BEEP_COLOR_STOP_PRESSED),
                                LV_STATE_PRESSED);
      lv_obj_set_style_text_color(button, lv_color_hex(BEEP_COLOR_TEXT_DARK),
                                  0);
    } else {
      lv_obj_set_style_bg_color(button, lv_color_hex(BEEP_COLOR_BLUE), 0);
      lv_obj_set_style_bg_color(button, lv_color_hex(BEEP_COLOR_BLUE_PRESSED),
                                LV_STATE_PRESSED);
      lv_obj_set_style_text_color(button, lv_color_white(), 0);
    }

    label = lv_obj_get_child(button, 0);
    if (label != NULL)
      lv_label_set_text_static(label, text);
  }
}

/* 删除待触发的单次播放完成定时器。 */
static void beep_cancel_single_timer(void) {
  if (g_single_timer != NULL) {
    lv_timer_del(g_single_timer);
    g_single_timer = NULL;
  }
}

/* 单次播放时长到期：把按钮复位为 Start。
 * repeat_count 为 1，回调结束后定时器由 LVGL 自动删除。 */
static void beep_single_done_cb(lv_timer_t *timer) {
  (void)timer;

  g_single_timer = NULL;
  g_beep_running = false;
  beep_refresh_action();
}

/* 启停切换：停止态按当前模式启动，运行中点击立即停止。
 * 单次播放时长已知，挂一个到期定时器自动复位按钮。 */
static void beep_toggle_event_cb(lv_event_t *event) {
  (void)event;

  if (g_beep_running) {
    beep_cancel_single_timer();
    stopBeep();
    g_beep_running = false;
  } else {
    beep_hw_start();
    g_beep_running = true;

    if (!g_loop_mode) {
      g_single_timer = lv_timer_create(beep_single_done_cb,
                                       (uint32_t)g_duration.value, NULL);
      lv_timer_set_repeat_count(g_single_timer, 1);
    }
  }

  beep_refresh_action();
}

/* ==================== 页面构建 ==================== */

/* 创建模式切换胶囊按钮。 */
static lv_obj_t *beep_create_pill(lv_obj_t *parent, const char *text,
                                  lv_coord_t x) {
  lv_obj_t *button = lv_btn_create(parent);

  lv_obj_remove_style_all(button);
  lv_obj_add_style(button, &g_styles[STYLE_PILL], 0);
  lv_obj_set_size(button, BEEP_PILL_WIDTH, BEEP_PILL_HEIGHT);
  lv_obj_set_pos(button, x, BEEP_PILL_Y);

  {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text_static(label, text);
    lv_obj_center(label);
  }

  return button;
}

/* 创建透明分区容器。 */
static lv_obj_t *beep_create_section(lv_obj_t *parent) {
  lv_obj_t *section = lv_obj_create(parent);

  lv_obj_remove_style_all(section);
  lv_obj_add_style(section, &g_styles[STYLE_SECTION], 0);
  lv_obj_set_size(section, BEEP_CARD_WIDTH, BEEP_CARD_HEIGHT);
  lv_obj_set_pos(section, 0, 0);
  lv_obj_clear_flag(section, LV_OBJ_FLAG_SCROLLABLE);
  return section;
}

/* 创建一行可调参数：透明容器内名称标签 + [− 值 +] 步进组（ui_param 组件）。
 * 单次模式的时长行和循环模式的频率、占空比行完全一致。 */
static void beep_create_param_row(lv_obj_t *parent, const char *name,
                                  ui_param_t *param, ui_step_btn_t *minus,
                                  ui_step_btn_t *plus, lv_coord_t y) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_t *name_label = lv_label_create(row);

  lv_obj_remove_style_all(row);
  lv_obj_add_style(row, &g_styles[STYLE_SECTION], 0);
  lv_obj_set_size(row, BEEP_CARD_WIDTH, ROW_HEIGHT);
  lv_obj_set_pos(row, 0, y);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_add_style(name_label, &g_styles[STYLE_NAME_LABEL], 0);
  lv_label_set_text_static(name_label, name);
  lv_obj_set_pos(name_label, ROW_NAME_X, ROW_TEXT_DY);

  ui_param_create_stepper(row, param, minus, plus);
}

/* 单次模式分区：时长参数行 + 启停切换按钮。 */
static void beep_create_single_section(lv_obj_t *card) {
  lv_obj_t *section = beep_create_section(card);

  g_section_single = section;

  beep_create_param_row(section, "Duration", &g_duration, &g_single_minus,
                        &g_single_plus, SINGLE_DURATION_ROW_Y);
  g_action_single = beep_create_action_btn(section, ACTION_Y_SINGLE);
  lv_obj_add_event_cb(g_action_single, beep_toggle_event_cb, LV_EVENT_CLICKED,
                      NULL);
}

/* 循环模式分区：频率、占空比两行 + 启停切换按钮。 */
static void beep_create_loop_section(lv_obj_t *card) {
  lv_obj_t *section = beep_create_section(card);

  g_section_loop = section;

  beep_create_param_row(section, "Frequency", &g_frequency, &g_freq_minus,
                        &g_freq_plus, LOOP_FREQ_ROW_Y);
  beep_create_param_row(section, "Duty", &g_duty, &g_duty_minus, &g_duty_plus,
                        LOOP_DUTY_ROW_Y);
  g_action_loop = beep_create_action_btn(section, ACTION_Y_LOOP);
  lv_obj_add_event_cb(g_action_loop, beep_toggle_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /* 默认显示单次模式，循环分区隐藏。 */
  lv_obj_add_flag(section, LV_OBJ_FLAG_HIDDEN);
}

/* 初始化蜂鸣器控制页面。 */
void view_beep_init(lv_obj_t *page, void *params) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t end_tick;
  lv_obj_t *card;

  (void)params;

  /* 页面可能因缓存淘汰被重建，复位为默认单次模式；
   * UI 与硬件状态需保持一致，重建时顺带停止仍在播放的蜂鸣。 */
  g_loop_mode = false;
  g_beep_running = false;
  beep_cancel_single_timer();
  stopBeep();

  /* 页面容器由路由器统一创建并定位，子控件按固定坐标摆放。 */
  beep_styles_init();

  /* 顶部模式切换按钮。 */
  g_pill_single = beep_create_pill(page, "Single", 0);
  g_pill_loop = beep_create_pill(page, "Loop",
                                 BEEP_PILL_WIDTH + BEEP_PILL_GAP);
  lv_obj_add_event_cb(g_pill_single, beep_pill_single_event_cb,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(g_pill_loop, beep_pill_loop_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /* 白色卡片承载两种模式的参数分区。 */
  card = lv_obj_create(page);
  lv_obj_remove_style_all(card);
  lv_obj_add_style(card, &g_styles[STYLE_CARD], 0);
  lv_obj_set_size(card, BEEP_CARD_WIDTH, BEEP_CARD_HEIGHT);
  lv_obj_set_pos(card, BEEP_CARD_X, BEEP_CARD_Y);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  beep_create_single_section(card);
  beep_create_loop_section(card);

  /* 默认单次模式。 */
  beep_refresh_pills();
  beep_refresh_action();

  end_tick = xTaskGetTickCount();
  printf("[beep] total=%lu ms\r\n",
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/*
 * 页面离开显示（转入缓存或压入历史栈）：暂停单次播放完成定时器。
 * 蜂鸣器硬件不受影响继续播放，只是按钮复位动作推迟到页面重新显示时，
 * 恢复后若时长已到期，LVGL 会在第一轮 handler 立即补发回调复位按钮。
 */
void view_beep_hide(lv_obj_t *page) {
  (void)page;

  if (g_single_timer)
    lv_timer_pause(g_single_timer);
}

/* 页面重新显示：恢复单次播放完成定时器。 */
void view_beep_show(lv_obj_t *page) {
  (void)page;

  if (g_single_timer)
    lv_timer_resume(g_single_timer);
}

/* 页面被缓存淘汰销毁前，清空本模块的控件引用，避免下次重建误判。 */
void view_beep_destroy(lv_obj_t *page) {
  (void)page;

  g_pill_single = NULL;
  g_pill_loop = NULL;
  g_section_single = NULL;
  g_section_loop = NULL;
  g_action_single = NULL;
  g_action_loop = NULL;
  g_beep_running = false;
  /* 定时器回调引用页面控件，销毁前必须删除。 */
  beep_cancel_single_timer();
  g_duration.label = NULL;
  g_frequency.label = NULL;
  g_duty.label = NULL;
  g_single_minus.repeats = 0;
  g_single_plus.repeats = 0;
  g_freq_minus.repeats = 0;
  g_freq_plus.repeats = 0;
  g_duty_minus.repeats = 0;
  g_duty_plus.repeats = 0;
}
