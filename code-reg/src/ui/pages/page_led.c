#include "page_led.h"
#include "../components/ui_param.h"
#include "led/task_led.h"
#include "lvgl.h"
#include "task.h"
#include <stdio.h>

/* ==================== 布局常量 ====================
 * 页面内容区 432 x 636（480 x 684 减去四周 24 内边距），
 * 全部使用固定坐标，避免运行时布局计算开销。
 */
#define LED_PILL_WIDTH 204
#define LED_PILL_HEIGHT 64
#define LED_PILL_GAP 24
#define LED_PILL_Y 0

/* 分区容器起始位置：紧贴顶部切换按钮下方，避免遮挡按钮点击。 */
#define LED_SECTION_Y 88
#define LED_SECTION_HEIGHT (636 - LED_SECTION_Y)

/* 独立控制分区：红灯卡片 + 绿灯卡片（坐标相对分区容器）。 */
#define RED_CARD_X 0
#define RED_CARD_Y 0
#define LED_CARD_WIDTH 432
#define RED_CARD_HEIGHT 372

#define GREEN_CARD_X 0
#define GREEN_CARD_Y 388
#define GREEN_CARD_HEIGHT 160

/* 组合特效分区卡片。 */
#define COMBO_CARD_X 0
#define COMBO_CARD_Y 0
#define COMBO_CARD_WIDTH 432
#define COMBO_CARD_HEIGHT 220

/* 卡片内统一边距。 */
#define CARD_NAME_X 24
#define CARD_TITLE_Y 18

/* 红灯卡片控件位置。 */
#define RED_CHIP_Y 54
#define RED_CHIP_WIDTH 180
#define RED_CHIP_HEIGHT 50
#define RED_CHIP_LEFT_X 24
#define RED_CHIP_RIGHT_X 228

/* 参数行容器尺寸（内部 [− 值 +] 布局由 ui_param 组件定义）。 */
#define PARAM_ROW_HEIGHT 48
#define PARAM_ROW_TEXT_DY 12

/* Brightness 行常显（Breath 模式下代表最大亮度），差值行仅 Breath 模式显示。 */
#define RED_BRIGHT_ROW_Y 116
#define RED_BREATH_ROW_Y 172
#define RED_TOGGLE_X 126
#define RED_TOGGLE_Y 300
#define RED_TOGGLE_WIDTH 180
#define RED_TOGGLE_HEIGHT 56

/* 绿灯卡片控件位置。 */
#define GREEN_CHIP_X 126
#define GREEN_CHIP_Y 60
#define GREEN_CHIP_WIDTH 180
#define GREEN_CHIP_HEIGHT 56

/* 组合特效卡片控件位置。 */
#define COMBO_CHIP_Y 60
#define COMBO_CHIP_WIDTH 120
#define COMBO_CHIP_HEIGHT 56
#define COMBO_CHIP_1_X 24
#define COMBO_CHIP_2_X 156
#define COMBO_CHIP_3_X 288
/* Interval 行仅 Flow 特效选中时显示。 */
#define COMBO_INTERVAL_Y 148

/* ==================== 颜色 ==================== */
#define LED_COLOR_BLUE 0x3A76F4
#define LED_COLOR_BLUE_PRESSED 0x2E5FCC
#define LED_COLOR_CHIP_BG 0xFFFFFF
#define LED_COLOR_CHIP_BORDER 0xC8C8C8
#define LED_COLOR_STOP_BG 0xE8E8E8
#define LED_COLOR_STOP_PRESSED 0xD5D5D5
#define LED_COLOR_TEXT_DARK 0x1A1A1A
#define LED_COLOR_RED_ON 0xE5484D
#define LED_COLOR_RED_ON_PRESSED 0xC93B40
#define LED_COLOR_GREEN_ON 0x2FBF5B
#define LED_COLOR_GREEN_ON_PRESSED 0x27A34B

/* ==================== 长按加速 ==================== */
/* 呼吸灯亮度差值（最低到最高的跨度）最小值（百分比）。 */
#define BREATH_DELTA_MIN 5

/* 组合特效选项（按卡片内从左到右的按钮顺序）。 */
typedef enum {
  COMBO_ON = 0, /* 双灯同时亮 */
  COMBO_OFF,    /* 双灯同时灭 */
  COMBO_FLOW,   /* 流水灯 */
  COMBO_COUNT
} led_combo_t;

/* 页面状态：分区选择、红灯模式、组合特效选择、绿灯开关、红灯运行中。 */
static bool g_combo_mode;
static bool g_red_breath;
static led_combo_t g_combo;
static bool g_green_on;
static bool g_red_running;

/* 页面级控件引用。 */
static lv_obj_t *g_pill_independent;
static lv_obj_t *g_pill_combo;
static lv_obj_t *g_section_independent;
static lv_obj_t *g_section_combo;
static lv_obj_t *g_red_chips[2]; /* 0 恒定亮度，1 呼吸灯 */
static lv_obj_t *g_combo_chips[COMBO_COUNT];
static lv_obj_t *g_red_button;
static lv_obj_t *g_green_button;
static lv_obj_t *g_breath_row;   /* Breath 模式下的亮度差值行容器 */
static lv_obj_t *g_interval_row; /* Flow 特效下的切换间隔行容器 */

/* ==================== 参数步进（ui_param 基础组件） ====================
 * 亮度下调后差值可能超过亮度：在 on_clamp 中收紧并刷新显示；
 * 运行中的硬件同步（PWM / 呼吸范围 / 流水间隔）在 on_changed 中处理。 */

static void led_brightness_clamp(ui_param_t *param);
static void led_delta_clamp(ui_param_t *param);
static void led_brightness_changed(ui_param_t *param);
static void led_delta_changed(ui_param_t *param);
static void led_interval_changed(ui_param_t *param);

/* 参数初始值与范围。亮度下限 5%，避免直接调到灭灯；
 * Breath 模式下 Brightness 作为最大亮度，差值决定最低亮度 = 最大 - 差值，
 * 差值范围 [5, 当前亮度]（上限随亮度动态变化，在 on_clamp 中夹紧）。 */
static ui_param_t g_brightness = {75, 5, 100, 5, "%",
                                  NULL, NULL, led_brightness_clamp,
                                  led_brightness_changed};
static ui_param_t g_breath_delta = {
    70, BREATH_DELTA_MIN, 100, 5, "%", NULL, NULL, led_delta_clamp,
    led_delta_changed};
static ui_param_t g_interval = {500, 100, 2000, 50, " ms",
                                NULL, NULL, NULL, led_interval_changed};

/* 步进按钮上下文（静态分配，页面销毁时清零）。 */
static ui_step_btn_t g_bright_minus = {NULL, -1, 0};
static ui_step_btn_t g_bright_plus = {NULL, 1, 0};
static ui_step_btn_t g_delta_minus = {NULL, -1, 0};
static ui_step_btn_t g_delta_plus = {NULL, 1, 0};
static ui_step_btn_t g_interval_minus = {NULL, -1, 0};
static ui_step_btn_t g_interval_plus = {NULL, 1, 0};

static void led_brightness_clamp(ui_param_t *param) {
  (void)param;
  if (g_breath_delta.value > g_brightness.value) {
    g_breath_delta.value = g_brightness.value;
    ui_param_update_label(&g_breath_delta);
  }
}

static void led_delta_clamp(ui_param_t *param) {
  /* 差值上限随最大亮度动态变化：不超过当前亮度，且不低于最小差值。 */
  if (param->value > g_brightness.value)
    param->value = g_brightness.value;
  if (param->value < BREATH_DELTA_MIN)
    param->value = BREATH_DELTA_MIN;
}

static void led_brightness_changed(ui_param_t *param) {
  (void)param;
  /* 恒定亮度模式运行中：立即作用到 PWM 占空比。 */
  if (g_red_running && !g_red_breath)
    led_set_light((uint8_t)g_brightness.value);
  /* 呼吸灯运行中：亮度即最大亮度，最低亮度 = 亮度 - 差值。 */
  if (g_red_running && g_red_breath)
    ledBreathSetRange((uint8_t)(g_brightness.value - g_breath_delta.value),
                      (uint8_t)g_brightness.value);
}

static void led_delta_changed(ui_param_t *param) {
  (void)param;
  if (g_red_running && g_red_breath)
    ledBreathSetRange((uint8_t)(g_brightness.value - g_breath_delta.value),
                      (uint8_t)g_brightness.value);
}

static void led_interval_changed(ui_param_t *param) {
  (void)param;
  /* 组合特效流水灯运行中：间隔调整立即作用到定时器。 */
  if (g_combo_mode && g_combo == COMBO_FLOW)
    ledFlowlightSetInterval((uint32_t)g_interval.value);
}

/* ==================== 共用样式 ====================
 * 所有自定义样式只在首次创建页面时构建一次，页面控件直接引用。
 */
enum {
  STYLE_CARD = 0,
  STYLE_SECTION,
  STYLE_PILL,
  STYLE_CHIP,
  STYLE_TOGGLE_BTN,
  STYLE_NAME_LABEL,
  STYLE_COUNT
};

static lv_style_t g_styles[STYLE_COUNT];
static bool g_styles_initialized;

static void led_styles_init(void) {
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
                          lv_color_hex(LED_COLOR_TEXT_DARK));

  /* 透明分区容器，用于整体显示 / 隐藏一种模式的全部控件。 */
  lv_style_init(&g_styles[STYLE_SECTION]);
  lv_style_set_bg_opa(&g_styles[STYLE_SECTION], LV_OPA_TRANSP);
  lv_style_set_border_width(&g_styles[STYLE_SECTION], 0);
  lv_style_set_pad_all(&g_styles[STYLE_SECTION], 0);

  /* 模式切换胶囊按钮（未选中外观，选中态在切换时直接覆盖颜色）。 */
  lv_style_init(&g_styles[STYLE_PILL]);
  lv_style_set_bg_color(&g_styles[STYLE_PILL], lv_color_hex(LED_COLOR_CHIP_BG));
  lv_style_set_bg_opa(&g_styles[STYLE_PILL], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_PILL], LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&g_styles[STYLE_PILL], 1);
  lv_style_set_border_color(&g_styles[STYLE_PILL],
                            lv_color_hex(LED_COLOR_CHIP_BORDER));
  lv_style_set_pad_all(&g_styles[STYLE_PILL], 0);
  lv_style_set_text_color(&g_styles[STYLE_PILL],
                          lv_color_hex(LED_COLOR_TEXT_DARK));

  /* 卡片内的小选项胶囊（红灯模式、组合特效）。 */
  lv_style_init(&g_styles[STYLE_CHIP]);
  lv_style_set_bg_color(&g_styles[STYLE_CHIP],
                        lv_color_hex(LED_COLOR_CHIP_BG));
  lv_style_set_bg_opa(&g_styles[STYLE_CHIP], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_CHIP], LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&g_styles[STYLE_CHIP], 1);
  lv_style_set_border_color(&g_styles[STYLE_CHIP],
                            lv_color_hex(LED_COLOR_CHIP_BORDER));
  lv_style_set_pad_all(&g_styles[STYLE_CHIP], 0);
  lv_style_set_text_color(&g_styles[STYLE_CHIP],
                          lv_color_hex(LED_COLOR_TEXT_DARK));

  /* 开关按钮（红灯 / 绿灯共用，关闭态外观，打开色在切换时覆盖）。 */
  lv_style_init(&g_styles[STYLE_TOGGLE_BTN]);
  lv_style_set_bg_color(&g_styles[STYLE_TOGGLE_BTN],
                        lv_color_hex(LED_COLOR_STOP_BG));
  lv_style_set_bg_opa(&g_styles[STYLE_TOGGLE_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STYLE_TOGGLE_BTN], LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&g_styles[STYLE_TOGGLE_BTN], 0);
  lv_style_set_pad_all(&g_styles[STYLE_TOGGLE_BTN], 0);
  lv_style_set_text_color(&g_styles[STYLE_TOGGLE_BTN],
                          lv_color_hex(LED_COLOR_TEXT_DARK));

  /* 参数名称文字（数值显示样式由 ui_param 组件提供）。 */
  lv_style_init(&g_styles[STYLE_NAME_LABEL]);
  lv_style_set_text_color(&g_styles[STYLE_NAME_LABEL],
                          lv_color_hex(LED_COLOR_TEXT_DARK));

  g_styles_initialized = true;
}

/* ==================== 参数行 ==================== */

/* 创建一行可调参数：透明容器内 name 标签 + [− 值 +] 步进组（ui_param 组件）。
 * 容器作为整体返回，便于按模式显示 / 隐藏。 */
static lv_obj_t *led_create_param_row(lv_obj_t *parent, const char *name,
                                      ui_param_t *param, ui_step_btn_t *minus,
                                      ui_step_btn_t *plus, lv_coord_t y) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_t *name_label = lv_label_create(row);

  lv_obj_remove_style_all(row);
  lv_obj_add_style(row, &g_styles[STYLE_SECTION], 0);
  lv_obj_set_size(row, LED_CARD_WIDTH, PARAM_ROW_HEIGHT);
  lv_obj_set_pos(row, 0, y);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_add_style(name_label, &g_styles[STYLE_NAME_LABEL], 0);
  lv_label_set_text_static(name_label, name);
  lv_obj_set_pos(name_label, CARD_NAME_X, PARAM_ROW_TEXT_DY);

  ui_param_create_stepper(row, param, minus, plus);
  return row;
}

/* 创建一个选项胶囊按钮（未选中外观，选中态由刷新函数覆盖）。 */
static lv_obj_t *led_create_chip(lv_obj_t *parent, const char *text,
                                 lv_coord_t x, lv_coord_t y, lv_coord_t w,
                                 lv_coord_t h) {
  lv_obj_t *button = lv_btn_create(parent);

  lv_obj_remove_style_all(button);
  lv_obj_add_style(button, &g_styles[STYLE_CHIP], 0);
  lv_obj_set_size(button, w, h);
  lv_obj_set_pos(button, x, y);

  {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text_static(label, text);
    lv_obj_center(label);
  }

  return button;
}

/* 按选中下标刷新一个胶囊按钮组的外观。 */
static void led_refresh_chip_group(lv_obj_t **chips, uint32_t count,
                                   uint32_t selected) {
  uint32_t i;

  for (i = 0; i < count; i++) {
    if (i == selected) {
      lv_obj_set_style_bg_color(chips[i], lv_color_hex(LED_COLOR_BLUE), 0);
      lv_obj_set_style_text_color(chips[i], lv_color_white(), 0);
      lv_obj_set_style_border_width(chips[i], 0, 0);
    } else {
      lv_obj_set_style_bg_color(chips[i], lv_color_hex(LED_COLOR_CHIP_BG), 0);
      lv_obj_set_style_text_color(chips[i],
                                  lv_color_hex(LED_COLOR_TEXT_DARK), 0);
      lv_obj_set_style_border_width(chips[i], 1, 0);
    }
  }
}

/* ==================== 模式切换 ==================== */

static void led_apply_red_state(void);
static void led_apply_combo_state(void);

/* 刷新顶部两个分区切换按钮的选中态外观。 */
static void led_refresh_pills(void) {
  lv_obj_t *active = g_combo_mode ? g_pill_combo : g_pill_independent;
  lv_obj_t *inactive = g_combo_mode ? g_pill_independent : g_pill_combo;

  lv_obj_set_style_bg_color(active, lv_color_hex(LED_COLOR_BLUE), 0);
  lv_obj_set_style_text_color(active, lv_color_white(), 0);
  lv_obj_set_style_border_width(active, 0, 0);

  lv_obj_set_style_bg_color(inactive, lv_color_hex(LED_COLOR_CHIP_BG), 0);
  lv_obj_set_style_text_color(inactive, lv_color_hex(LED_COLOR_TEXT_DARK), 0);
  lv_obj_set_style_border_width(inactive, 1, 0);
}

static void led_set_mode(bool combo_mode) {
  if (g_combo_mode == combo_mode)
    return;

  g_combo_mode = combo_mode;

  if (combo_mode) {
    lv_obj_add_flag(g_section_independent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_section_combo, LV_OBJ_FLAG_HIDDEN);
    /* 进入组合特效：立即按当前选中特效输出（默认 On）。 */
    led_apply_combo_state();
  } else {
    lv_obj_clear_flag(g_section_independent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_section_combo, LV_OBJ_FLAG_HIDDEN);
    /* 回到独立控制：按红灯 / 绿灯状态重新输出，覆盖组合特效结果。 */
    led_apply_red_state();
  }

  led_refresh_pills();
}

static void led_pill_independent_event_cb(lv_event_t *event) {
  (void)event;
  led_set_mode(false);
}

static void led_pill_combo_event_cb(lv_event_t *event) {
  (void)event;
  led_set_mode(true);
}

/* ==================== 选项回调 ==================== */

static void led_red_chip_event_cb(lv_event_t *event) {
  bool breath = lv_event_get_user_data(event) != NULL;

  if (g_red_breath == breath)
    return;

  g_red_breath = breath;
  led_refresh_chip_group(g_red_chips, 2, breath ? 1 : 0);

  /* Breath 模式显示亮度差值行，Steady 模式隐藏（Brightness 行常显）。 */
  if (breath)
    lv_obj_clear_flag(g_breath_row, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(g_breath_row, LV_OBJ_FLAG_HIDDEN);

  /* 红灯正在运行时切换模式：立即按新模式生效。 */
  if (g_red_running)
    led_apply_red_state();
}

/* 组合特效：点击选项立即生效，Interval 行仅 Flow 选中时显示。 */
static void led_combo_chip_event_cb(lv_event_t *event) {
  led_combo_t combo = (led_combo_t)(lv_uintptr_t)lv_event_get_user_data(event);

  if (g_combo == combo)
    return;

  g_combo = combo;
  led_refresh_chip_group(g_combo_chips, COMBO_COUNT, (uint32_t)combo);

  if (combo == COMBO_FLOW)
    lv_obj_clear_flag(g_interval_row, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(g_interval_row, LV_OBJ_FLAG_HIDDEN);

  led_apply_combo_state();
}

/* 刷新绿灯开关按钮的外观与文字。 */
static void led_refresh_green_button(void) {
  lv_obj_t *label = lv_obj_get_child(g_green_button, 0);

  if (g_green_on) {
    lv_obj_set_style_bg_color(g_green_button, lv_color_hex(LED_COLOR_GREEN_ON),
                              0);
    lv_obj_set_style_bg_color(g_green_button,
                              lv_color_hex(LED_COLOR_GREEN_ON_PRESSED),
                              LV_STATE_PRESSED);
    lv_obj_set_style_text_color(g_green_button, lv_color_white(), 0);
  } else {
    lv_obj_set_style_bg_color(g_green_button,
                              lv_color_hex(LED_COLOR_STOP_BG), 0);
    lv_obj_set_style_bg_color(
        g_green_button, lv_color_hex(LED_COLOR_STOP_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(g_green_button,
                                lv_color_hex(LED_COLOR_TEXT_DARK), 0);
  }

  if (label != NULL)
    lv_label_set_text_static(label, g_green_on ? "Green On" : "Green Off");
}

/* 绿灯开关：点击立即切换硬件状态。 */
static void led_green_event_cb(lv_event_t *event) {
  (void)event;

  g_green_on = !g_green_on;

  if (g_green_on)
    led_on(LED_GREEN);
  else
    led_off(LED_GREEN);

  led_refresh_green_button();
}

/* ==================== 启动 / 停止 ==================== */

/* 按当前状态把红灯输出到硬件：
 * 运行时恒定亮度直接设 PWM，呼吸灯交给后台任务；停止时熄灭红灯。
 * 后台模式切换会顺带熄灭两个灯，因此这里同时重新应用绿灯开关状态。 */
static void led_apply_red_state(void) {
  if (!g_red_running) {
    setLedMode(LED_MODE_OFF);
    /* LED_MODE_OFF 只停定时器不关灯，红灯需要显式熄灭。 */
    led_off(LED_RED);
  } else if (g_red_breath) {
    /* 最低亮度 = 最大亮度(Brightness) - 差值。 */
    ledBreathSetRange((uint8_t)(g_brightness.value - g_breath_delta.value),
                      (uint8_t)g_brightness.value);
    setLedMode(LED_MODE_BREATH);
  } else {
    /* 先停掉呼吸灯任务，否则它会持续覆盖 PWM 占空比。 */
    setLedMode(LED_MODE_OFF);
    led_set_light((uint8_t)g_brightness.value);
  }

  if (g_green_on)
    led_on(LED_GREEN);
  else
    led_off(LED_GREEN);
}

/* 刷新红灯开关按钮的外观与文字。 */
static void led_refresh_red_button(void) {
  lv_obj_t *label = lv_obj_get_child(g_red_button, 0);

  if (g_red_running) {
    lv_obj_set_style_bg_color(g_red_button, lv_color_hex(LED_COLOR_RED_ON), 0);
    lv_obj_set_style_bg_color(g_red_button,
                              lv_color_hex(LED_COLOR_RED_ON_PRESSED),
                              LV_STATE_PRESSED);
    lv_obj_set_style_text_color(g_red_button, lv_color_white(), 0);
  } else {
    lv_obj_set_style_bg_color(g_red_button, lv_color_hex(LED_COLOR_STOP_BG), 0);
    lv_obj_set_style_bg_color(g_red_button,
                              lv_color_hex(LED_COLOR_STOP_PRESSED),
                              LV_STATE_PRESSED);
    lv_obj_set_style_text_color(g_red_button, lv_color_hex(LED_COLOR_TEXT_DARK),
                                0);
  }

  if (label != NULL)
    lv_label_set_text_static(label, g_red_running ? "Red On" : "Red Off");
}

/* 红灯开关：点击切换运行状态，按当前 Steady / Breath 模式生效。 */
static void led_red_toggle_event_cb(lv_event_t *event) {
  (void)event;

  g_red_running = !g_red_running;

  led_apply_red_state();
  led_refresh_red_button();
}

/* 组合特效：按当前选中项立即输出，流水灯使用当前切换间隔。 */
static void led_apply_combo_state(void) {
  switch (g_combo) {
  case COMBO_ON:
    setLedMode(LED_MODE_ON);
    break;
  case COMBO_OFF:
    setLedMode(LED_MODE_OFF);
    /* LED_MODE_OFF 只停定时器不关灯，两个灯都需要显式熄灭。 */
    led_off(LED_RED);
    led_off(LED_GREEN);
    break;
  case COMBO_FLOW:
  default:
    ledFlowlightSetInterval((uint32_t)g_interval.value);
    setLedMode(LED_MODE_FLOW);
    break;
  }
}

/* ==================== 页面构建 ==================== */

/* 创建分区切换胶囊按钮。 */
static lv_obj_t *led_create_pill(lv_obj_t *parent, const char *text,
                                 lv_coord_t x) {
  lv_obj_t *button = lv_btn_create(parent);

  lv_obj_remove_style_all(button);
  lv_obj_add_style(button, &g_styles[STYLE_PILL], 0);
  lv_obj_set_size(button, LED_PILL_WIDTH, LED_PILL_HEIGHT);
  lv_obj_set_pos(button, x, LED_PILL_Y);

  {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text_static(label, text);
    lv_obj_center(label);
  }

  return button;
}

/* 创建白色圆角卡片。 */
static lv_obj_t *led_create_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                 lv_coord_t w, lv_coord_t h) {
  lv_obj_t *card = lv_obj_create(parent);

  lv_obj_remove_style_all(card);
  lv_obj_add_style(card, &g_styles[STYLE_CARD], 0);
  lv_obj_set_size(card, w, h);
  lv_obj_set_pos(card, x, y);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

/* 创建透明分区容器，承载一个模式下的全部控件。 */
static lv_obj_t *led_create_section(lv_obj_t *parent) {
  lv_obj_t *section = lv_obj_create(parent);

  lv_obj_remove_style_all(section);
  lv_obj_add_style(section, &g_styles[STYLE_SECTION], 0);
  lv_obj_set_size(section, LED_CARD_WIDTH, LED_SECTION_HEIGHT);
  lv_obj_set_pos(section, 0, LED_SECTION_Y);
  lv_obj_clear_flag(section, LV_OBJ_FLAG_SCROLLABLE);
  return section;
}

/* 独立控制分区：红灯卡片（模式 + 亮度 + 启停）与绿灯卡片（开关）。 */
static void led_create_independent_section(lv_obj_t *page) {
  lv_obj_t *section = led_create_section(page);
  lv_obj_t *card;

  g_section_independent = section;

  /* 红灯卡片。 */
  card = led_create_card(section, RED_CARD_X, RED_CARD_Y, LED_CARD_WIDTH,
                         RED_CARD_HEIGHT);

  {
    lv_obj_t *title = lv_label_create(card);
    lv_obj_add_style(title, &g_styles[STYLE_NAME_LABEL], 0);
    lv_label_set_text_static(title, "Red LED");
    lv_obj_set_pos(title, CARD_NAME_X, CARD_TITLE_Y);
  }

  g_red_chips[0] = led_create_chip(card, "Steady", RED_CHIP_LEFT_X, RED_CHIP_Y,
                                   RED_CHIP_WIDTH, RED_CHIP_HEIGHT);
  g_red_chips[1] = led_create_chip(card, "Breath", RED_CHIP_RIGHT_X,
                                   RED_CHIP_Y, RED_CHIP_WIDTH,
                                   RED_CHIP_HEIGHT);
  lv_obj_add_event_cb(g_red_chips[0], led_red_chip_event_cb, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_add_event_cb(g_red_chips[1], led_red_chip_event_cb, LV_EVENT_CLICKED,
                      (void *)1);

  /* Brightness 行常显：Steady 下为恒定亮度，Breath 下为最大亮度。 */
  led_create_param_row(card, "Brightness", &g_brightness, &g_bright_minus,
                       &g_bright_plus, RED_BRIGHT_ROW_Y);

  /* Breath 模式专属：亮度差值行（最低 = Brightness - 差值），默认隐藏。 */
  g_breath_row =
      led_create_param_row(card, "Delta", &g_breath_delta, &g_delta_minus,
                           &g_delta_plus, RED_BREATH_ROW_Y);
  lv_obj_add_flag(g_breath_row, LV_OBJ_FLAG_HIDDEN);

  {
    g_red_button = lv_btn_create(card);
    lv_obj_t *label;
    lv_obj_remove_style_all(g_red_button);
    lv_obj_add_style(g_red_button, &g_styles[STYLE_TOGGLE_BTN], 0);
    lv_obj_set_size(g_red_button, RED_TOGGLE_WIDTH, RED_TOGGLE_HEIGHT);
    lv_obj_set_pos(g_red_button, RED_TOGGLE_X, RED_TOGGLE_Y);
    label = lv_label_create(g_red_button);
    lv_label_set_text_static(label, "Red Off");
    lv_obj_center(label);
    lv_obj_add_event_cb(g_red_button, led_red_toggle_event_cb,
                        LV_EVENT_CLICKED, NULL);
  }

  /* 绿灯卡片。 */
  card = led_create_card(section, GREEN_CARD_X, GREEN_CARD_Y, LED_CARD_WIDTH,
                         GREEN_CARD_HEIGHT);

  {
    lv_obj_t *title = lv_label_create(card);
    lv_obj_add_style(title, &g_styles[STYLE_NAME_LABEL], 0);
    lv_label_set_text_static(title, "Green LED");
    lv_obj_set_pos(title, CARD_NAME_X, CARD_TITLE_Y);
  }

  g_green_button = lv_btn_create(card);
  lv_obj_remove_style_all(g_green_button);
  lv_obj_add_style(g_green_button, &g_styles[STYLE_TOGGLE_BTN], 0);
  lv_obj_set_size(g_green_button, GREEN_CHIP_WIDTH, GREEN_CHIP_HEIGHT);
  lv_obj_set_pos(g_green_button, GREEN_CHIP_X, GREEN_CHIP_Y);
  {
    lv_obj_t *label = lv_label_create(g_green_button);
    lv_label_set_text_static(label, "Green Off");
    lv_obj_center(label);
  }
  lv_obj_add_event_cb(g_green_button, led_green_event_cb, LV_EVENT_CLICKED,
                      NULL);
}

/* 组合特效分区：On / Off / Flow 三个选项，点击立即生效，Flow 显示切换间隔。 */
static void led_create_combo_section(lv_obj_t *page) {
  lv_obj_t *section = led_create_section(page);
  lv_obj_t *card;
  lv_obj_t *title;

  g_section_combo = section;

  card = led_create_card(section, COMBO_CARD_X, COMBO_CARD_Y, COMBO_CARD_WIDTH,
                         COMBO_CARD_HEIGHT);

  title = lv_label_create(card);
  lv_obj_add_style(title, &g_styles[STYLE_NAME_LABEL], 0);
  lv_label_set_text_static(title, "Combo Effect");
  lv_obj_set_pos(title, CARD_NAME_X, CARD_TITLE_Y);

  g_combo_chips[COMBO_ON] = led_create_chip(card, "On", COMBO_CHIP_1_X,
                                            COMBO_CHIP_Y, COMBO_CHIP_WIDTH,
                                            COMBO_CHIP_HEIGHT);
  g_combo_chips[COMBO_OFF] = led_create_chip(card, "Off", COMBO_CHIP_2_X,
                                             COMBO_CHIP_Y, COMBO_CHIP_WIDTH,
                                             COMBO_CHIP_HEIGHT);
  g_combo_chips[COMBO_FLOW] = led_create_chip(card, "Flow", COMBO_CHIP_3_X,
                                              COMBO_CHIP_Y, COMBO_CHIP_WIDTH,
                                              COMBO_CHIP_HEIGHT);
  lv_obj_add_event_cb(g_combo_chips[COMBO_ON], led_combo_chip_event_cb,
                      LV_EVENT_CLICKED, (void *)(lv_uintptr_t)COMBO_ON);
  lv_obj_add_event_cb(g_combo_chips[COMBO_OFF], led_combo_chip_event_cb,
                      LV_EVENT_CLICKED, (void *)(lv_uintptr_t)COMBO_OFF);
  lv_obj_add_event_cb(g_combo_chips[COMBO_FLOW], led_combo_chip_event_cb,
                      LV_EVENT_CLICKED, (void *)(lv_uintptr_t)COMBO_FLOW);

  /* Interval 行仅 Flow 选中时显示，默认隐藏（默认特效为 On）。 */
  g_interval_row =
      led_create_param_row(card, "Interval", &g_interval, &g_interval_minus,
                           &g_interval_plus, COMBO_INTERVAL_Y);
  lv_obj_add_flag(g_interval_row, LV_OBJ_FLAG_HIDDEN);

  /* 默认显示独立控制分区，组合特效分区隐藏。 */
  lv_obj_add_flag(section, LV_OBJ_FLAG_HIDDEN);
}

/* 初始化 LED 控制页面。 */
void view_led_init(lv_obj_t *page, void *params) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t end_tick;

  (void)params;

  /* 页面可能因缓存淘汰被重建，各项状态复位为默认值。 */
  g_combo_mode = false;
  g_red_breath = false;
  g_combo = COMBO_ON;
  g_green_on = false;
  g_red_running = false;

  /* 页面容器由路由器统一创建并定位，子控件按固定坐标摆放。 */
  led_styles_init();

  /* 顶部分区切换按钮。 */
  g_pill_independent = led_create_pill(page, "Independent", 0);
  g_pill_combo =
      led_create_pill(page, "Combo", LED_PILL_WIDTH + LED_PILL_GAP);
  lv_obj_add_event_cb(g_pill_independent, led_pill_independent_event_cb,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(g_pill_combo, led_pill_combo_event_cb, LV_EVENT_CLICKED,
                      NULL);

  led_create_independent_section(page);
  led_create_combo_section(page);

  led_refresh_pills();
  led_refresh_chip_group(g_red_chips, 2, 0);
  led_refresh_chip_group(g_combo_chips, COMBO_COUNT, (uint32_t)g_combo);
  led_refresh_red_button();
  led_refresh_green_button();

  /* 页面重建后状态复位为全关，把硬件同步到一致，避免灯还亮着但 UI 显示关。 */
  led_apply_red_state();

  end_tick = xTaskGetTickCount();
  printf("[led] total=%lu ms\r\n",
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/* 页面被缓存淘汰销毁前，清空本模块的控件引用，避免下次重建误判。 */
void view_led_destroy(lv_obj_t *page) {
  uint32_t i;

  (void)page;

  g_pill_independent = NULL;
  g_pill_combo = NULL;
  g_section_independent = NULL;
  g_section_combo = NULL;
  g_red_button = NULL;
  g_green_button = NULL;
  g_breath_row = NULL;
  g_interval_row = NULL;
  g_brightness.label = NULL;
  g_breath_delta.label = NULL;
  g_interval.label = NULL;

  for (i = 0; i < 2; i++)
    g_red_chips[i] = NULL;
  for (i = 0; i < COMBO_COUNT; i++)
    g_combo_chips[i] = NULL;

  g_bright_minus.repeats = 0;
  g_bright_plus.repeats = 0;
  g_delta_minus.repeats = 0;
  g_delta_plus.repeats = 0;
  g_interval_minus.repeats = 0;
  g_interval_plus.repeats = 0;
}
