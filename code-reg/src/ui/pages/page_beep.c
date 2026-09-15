#include "page_beep.h"
#include "../common/ui_param.h"
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

/* 参数行统一布局（相对 section 容器）：
 * 名称在左上、数值在名称下方、减 / 加按钮在右侧。
 * 单次模式的时长行与循环模式的频率、占空比行样式完全一致。 */
#define ROW_NAME_X 24
#define ROW_STEP_WIDTH 90
#define ROW_STEP_HEIGHT 64
#define ROW_MINUS_X 228
#define ROW_PLUS_X 330

/* 单次模式：时长参数行放在第一行位置。 */
#define SINGLE_NAME_Y 28
#define SINGLE_VALUE_Y 62
#define SINGLE_STEP_Y 44

/* 循环模式：频率、占空比两行。 */
#define LOOP_FREQ_NAME_Y 28
#define LOOP_FREQ_VALUE_Y 62
#define LOOP_FREQ_STEP_Y 44
#define LOOP_DUTY_NAME_Y 176
#define LOOP_DUTY_VALUE_Y 210
#define LOOP_DUTY_STEP_Y 192

/* 启动 / 停止按钮位置（相对 section 容器，两种模式一致）。 */
#define ACTION_WIDTH 180
#define ACTION_HEIGHT 72
#define ACTION_START_X 30
#define ACTION_STOP_X 222
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

/* ==================== 长按加速 ==================== */
/* 可调参数与步进按钮：复用 ui_param 基础组件。 */
static ui_param_t g_duration = {100, 10, 10000, 10, "ms", "%ld %s",
                                NULL, NULL, NULL};
static ui_param_t g_frequency = {50, 1, 100, 1, "Hz", "%ld %s",
                                 NULL, NULL, NULL};
static ui_param_t g_duty = {50, 5, 95, 1, "%", "%ld %s",
                            NULL, NULL, NULL};

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
static bool g_loop_mode;

/* ==================== 共用样式 ====================
 * 所有自定义样式只在首次创建页面时构建一次，页面控件直接引用。
 */
enum {
  STYLE_CARD = 0,
  STYLE_SECTION,
  STYLE_PILL,
  STYLE_START_BTN,
  STYLE_STOP_BTN,
  STYLE_NAME_LABEL,
  STYLE_VALUE_LABEL,
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

  /* 启动按钮：蓝色。 */
  lv_style_init(&g_styles[STYLE_START_BTN]);
  lv_style_set_bg_color(&g_styles[STYLE_START_BTN],
                        lv_color_hex(BEEP_COLOR_BLUE));
  lv_style_set_bg_opa(&g_styles[STYLE_START_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_START_BTN], 12);
  lv_style_set_border_width(&g_styles[STYLE_START_BTN], 0);
  lv_style_set_pad_all(&g_styles[STYLE_START_BTN], 0);
  lv_style_set_text_color(&g_styles[STYLE_START_BTN], lv_color_white());

  /* 停止按钮：浅灰。 */
  lv_style_init(&g_styles[STYLE_STOP_BTN]);
  lv_style_set_bg_color(&g_styles[STYLE_STOP_BTN],
                        lv_color_hex(BEEP_COLOR_STOP_BG));
  lv_style_set_bg_opa(&g_styles[STYLE_STOP_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_STOP_BTN], 12);
  lv_style_set_border_width(&g_styles[STYLE_STOP_BTN], 0);
  lv_style_set_pad_all(&g_styles[STYLE_STOP_BTN], 0);
  lv_style_set_text_color(&g_styles[STYLE_STOP_BTN],
                          lv_color_hex(BEEP_COLOR_TEXT_DARK));

  /* 参数名称与数值文字。 */
  lv_style_init(&g_styles[STYLE_NAME_LABEL]);
  lv_style_set_text_color(&g_styles[STYLE_NAME_LABEL],
                          lv_color_hex(BEEP_COLOR_TEXT_DARK));

  lv_style_init(&g_styles[STYLE_VALUE_LABEL]);
  lv_style_set_text_color(&g_styles[STYLE_VALUE_LABEL],
                          lv_color_hex(BEEP_COLOR_TEXT_DARK));

  g_styles_initialized = true;
}

/* ==================== 数值显示与步进 ====================
 * 步进机制（短按 / 长按加速 / 范围夹紧）由 ui_param 基础组件提供，
 * 蜂鸣器页面仅保留自身布局：名称与数值在左侧堆叠，− / + 按钮在右侧。 */

/* 创建启动 / 停止按钮。 */
static lv_obj_t *beep_create_action_btn(lv_obj_t *parent, const char *text,
                                        bool is_start, lv_coord_t x,
                                        lv_coord_t y) {
  lv_obj_t *button = lv_btn_create(parent);

  lv_obj_remove_style_all(button);
  lv_obj_add_style(button,
                   is_start ? &g_styles[STYLE_START_BTN]
                            : &g_styles[STYLE_STOP_BTN],
                   0);
  lv_obj_set_size(button, ACTION_WIDTH, ACTION_HEIGHT);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_style_bg_color(
      button, lv_color_hex(is_start ? BEEP_COLOR_BLUE_PRESSED
                                    : BEEP_COLOR_STOP_PRESSED),
      LV_STATE_PRESSED);

  {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text_static(label, text);
    lv_obj_center(label);
  }

  return button;
}

/* 创建参数名称 + 数值文本，两者都按 x 左对齐摆放。 */
static void beep_create_value_text(lv_obj_t *parent, const char *name,
                                   lv_coord_t name_y, lv_coord_t value_y,
                                   lv_obj_t **value_label_out) {
  lv_obj_t *name_label = lv_label_create(parent);
  lv_obj_t *value_label = lv_label_create(parent);

  lv_obj_add_style(name_label, &g_styles[STYLE_NAME_LABEL], 0);
  lv_label_set_text_static(name_label, name);
  lv_obj_set_pos(name_label, ROW_NAME_X, name_y);

  lv_obj_add_style(value_label, &g_styles[STYLE_VALUE_LABEL], 0);
  lv_obj_set_pos(value_label, ROW_NAME_X, value_y);

  *value_label_out = value_label;
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

static void beep_start_event_cb(lv_event_t *event) {
  (void)event;

  if (g_loop_mode) {
    /* 频率参数暂未传给底层驱动（GPIO 开关），按周期折算播放和间隔时长。 */
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

    playBeepLoop((uint32_t)play_ms, (uint32_t)stop_ms);
  } else {
    playBeep((uint32_t)g_duration.value);
  }
}

static void beep_stop_event_cb(lv_event_t *event) {
  (void)event;
  stopBeep();
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

/* 创建一组参数行：名称 + 数值 + 右侧减 / 加按钮（步进按钮由 ui_param 组件创建）。
 * 单次模式的时长行和循环模式的频率、占空比行完全一致。 */
static void beep_create_param_row(lv_obj_t *parent, const char *name,
                                  lv_coord_t name_y, lv_coord_t value_y,
                                  lv_coord_t step_y, ui_param_t *param,
                                  ui_step_btn_t *minus, ui_step_btn_t *plus) {
  minus->param = param;
  plus->param = param;

  beep_create_value_text(parent, name, name_y, value_y, &param->label);
  ui_param_update_label(param);
  ui_param_create_step_btn(parent, "-", minus, ROW_MINUS_X, step_y,
                           ROW_STEP_WIDTH, ROW_STEP_HEIGHT);
  ui_param_create_step_btn(parent, "+", plus, ROW_PLUS_X, step_y,
                           ROW_STEP_WIDTH, ROW_STEP_HEIGHT);
}

/* 创建启动 / 停止按钮行。 */
static void beep_create_action_row(lv_obj_t *parent, lv_coord_t y) {
  lv_obj_t *start =
      beep_create_action_btn(parent, "Start", true, ACTION_START_X, y);
  lv_obj_t *stop =
      beep_create_action_btn(parent, "Stop", false, ACTION_STOP_X, y);

  lv_obj_add_event_cb(start, beep_start_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(stop, beep_stop_event_cb, LV_EVENT_CLICKED, NULL);
}

/* 单次模式分区：时长参数行 + 启动 / 停止。 */
static void beep_create_single_section(lv_obj_t *card) {
  lv_obj_t *section = beep_create_section(card);

  g_section_single = section;

  beep_create_param_row(section, "Duration", SINGLE_NAME_Y, SINGLE_VALUE_Y,
                        SINGLE_STEP_Y, &g_duration, &g_single_minus,
                        &g_single_plus);
  beep_create_action_row(section, ACTION_Y_SINGLE);
}

/* 循环模式分区：频率、占空比两行 + 启动 / 停止。 */
static void beep_create_loop_section(lv_obj_t *card) {
  lv_obj_t *section = beep_create_section(card);

  g_section_loop = section;

  beep_create_param_row(section, "Frequency", LOOP_FREQ_NAME_Y,
                        LOOP_FREQ_VALUE_Y, LOOP_FREQ_STEP_Y, &g_frequency,
                        &g_freq_minus, &g_freq_plus);
  beep_create_param_row(section, "Duty", LOOP_DUTY_NAME_Y, LOOP_DUTY_VALUE_Y,
                        LOOP_DUTY_STEP_Y, &g_duty, &g_duty_minus,
                        &g_duty_plus);
  beep_create_action_row(section, ACTION_Y_LOOP);

  /* 默认显示单次模式，循环分区隐藏。 */
  lv_obj_add_flag(section, LV_OBJ_FLAG_HIDDEN);
}

/* 初始化蜂鸣器控制页面。 */
void view_beep_init(lv_obj_t *page, void *params) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t end_tick;
  lv_obj_t *card;

  (void)params;

  /* 页面可能因缓存淘汰被重建，模式状态复位为默认的单次模式。 */
  g_loop_mode = false;

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
  g_loop_mode = false;
  beep_refresh_pills();

  end_tick = xTaskGetTickCount();
  printf("[beep] total=%lu ms\r\n",
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/* 页面被缓存淘汰销毁前，清空本模块的控件引用，避免下次重建误判。 */
void view_beep_destroy(lv_obj_t *page) {
  (void)page;

  g_pill_single = NULL;
  g_pill_loop = NULL;
  g_section_single = NULL;
  g_section_loop = NULL;
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
