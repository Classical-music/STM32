#include "ui_param.h"

/* 长按重复时步进倍率随重复次数线性增长，封顶后保持最快。 */
#define UI_STEP_ACCEL_MAX_MULT 200

/* 步进组配色（蓝色按钮 + 深色数值）。 */
#define UI_COLOR_BLUE 0x3A76F4
#define UI_COLOR_BLUE_PRESSED 0x2E5FCC
#define UI_COLOR_TEXT_DARK 0x1A1A1A

/* [− 值 +] 横向步进组内部布局（parent 宽按 432 设计）。 */
#define UI_MINUS_X 170
#define UI_VALUE_X 254
#define UI_VALUE_WIDTH 76
#define UI_PLUS_X 344
#define UI_STEP_WIDTH 64
#define UI_STEP_HEIGHT 48
#define UI_TEXT_DY 12

enum { UI_STYLE_STEP_BTN = 0, UI_STYLE_VALUE_LABEL, UI_STYLE_COUNT };

static lv_style_t g_ui_styles[UI_STYLE_COUNT];
static bool g_ui_styles_initialized;

static void ui_param_styles_init(void) {
  if (g_ui_styles_initialized)
    return;

  lv_style_init(&g_ui_styles[UI_STYLE_STEP_BTN]);
  lv_style_set_bg_color(&g_ui_styles[UI_STYLE_STEP_BTN],
                        lv_color_hex(UI_COLOR_BLUE));
  lv_style_set_bg_opa(&g_ui_styles[UI_STYLE_STEP_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_ui_styles[UI_STYLE_STEP_BTN], 12);
  lv_style_set_border_width(&g_ui_styles[UI_STYLE_STEP_BTN], 0);
  lv_style_set_pad_all(&g_ui_styles[UI_STYLE_STEP_BTN], 0);
  lv_style_set_text_color(&g_ui_styles[UI_STYLE_STEP_BTN], lv_color_white());

  lv_style_init(&g_ui_styles[UI_STYLE_VALUE_LABEL]);
  lv_style_set_text_color(&g_ui_styles[UI_STYLE_VALUE_LABEL],
                          lv_color_hex(UI_COLOR_TEXT_DARK));

  g_ui_styles_initialized = true;
}

void ui_param_update_label(ui_param_t *param) {
  if (param->label == NULL)
    return;

  if (param->fmt != NULL)
    lv_label_set_text_fmt(param->label, param->fmt, (long)param->value,
                          param->unit);
  else
    lv_label_set_text_fmt(param->label, "%ld%s", (long)param->value,
                          param->unit);
}

/* 按方向调整参数值：通用范围夹紧 → on_clamp 跨参数约束 → 提交并回调。 */
static void ui_param_apply_step(ui_step_btn_t *btn, int32_t mult) {
  ui_param_t *param = btn->param;
  int32_t delta = param->step * mult * btn->dir;
  int32_t old_value = param->value;

  if (delta > 0 && param->value <= param->max - delta)
    param->value += delta;
  else if (delta > 0)
    param->value = param->max;
  else if (delta < 0 && param->value >= param->min - delta)
    param->value += delta;
  else if (delta < 0)
    param->value = param->min;

  if (param->on_clamp != NULL)
    param->on_clamp(param);

  if (param->value == old_value)
    return;

  ui_param_update_label(param);

  if (param->on_changed != NULL)
    param->on_changed(param);
}

/* 步进按钮事件：按下清零加速计数，短按走一步，长按重复按倍率加速。 */
static void ui_param_step_event_cb(lv_event_t *event) {
  ui_step_btn_t *btn = (ui_step_btn_t *)lv_event_get_user_data(event);
  lv_event_code_t code = lv_event_get_code(event);
  uint16_t mult;

  switch (code) {
  case LV_EVENT_PRESSED:
    btn->repeats = 0;
    break;
  case LV_EVENT_SHORT_CLICKED:
    ui_param_apply_step(btn, 1);
    break;
  case LV_EVENT_LONG_PRESSED_REPEAT:
    btn->repeats++;
    mult = (uint16_t)btn->repeats + 1;
    if (mult > UI_STEP_ACCEL_MAX_MULT)
      mult = UI_STEP_ACCEL_MAX_MULT;
    ui_param_apply_step(btn, (int32_t)mult);
    break;
  default:
    break;
  }
}

lv_obj_t *ui_param_create_step_btn(lv_obj_t *parent, const char *text,
                                   ui_step_btn_t *btn, lv_coord_t x,
                                   lv_coord_t y, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *button;
  lv_obj_t *label;

  ui_param_styles_init();

  button = lv_btn_create(parent);
  lv_obj_remove_style_all(button);
  lv_obj_add_style(button, &g_ui_styles[UI_STYLE_STEP_BTN], 0);
  lv_obj_set_size(button, w, h);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_style_bg_color(button, lv_color_hex(UI_COLOR_BLUE_PRESSED),
                            LV_STATE_PRESSED);

  label = lv_label_create(button);
  lv_label_set_text_static(label, text);
  lv_obj_center(label);

  lv_obj_add_event_cb(button, ui_param_step_event_cb, LV_EVENT_PRESSED, btn);
  lv_obj_add_event_cb(button, ui_param_step_event_cb, LV_EVENT_SHORT_CLICKED,
                      btn);
  lv_obj_add_event_cb(button, ui_param_step_event_cb,
                      LV_EVENT_LONG_PRESSED_REPEAT, btn);
  return button;
}

void ui_param_create_stepper(lv_obj_t *parent, ui_param_t *param,
                             ui_step_btn_t *minus, ui_step_btn_t *plus) {
  lv_obj_t *value_label;

  ui_param_styles_init();

  minus->param = param;
  plus->param = param;
  param->label = NULL;

  ui_param_create_step_btn(parent, "-", minus, UI_MINUS_X, 0, UI_STEP_WIDTH,
                           UI_STEP_HEIGHT);
  ui_param_create_step_btn(parent, "+", plus, UI_PLUS_X, 0, UI_STEP_WIDTH,
                           UI_STEP_HEIGHT);

  value_label = lv_label_create(parent);
  lv_obj_add_style(value_label, &g_ui_styles[UI_STYLE_VALUE_LABEL], 0);
  lv_obj_set_size(value_label, UI_VALUE_WIDTH, LV_SIZE_CONTENT);
  lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(value_label, UI_VALUE_X, UI_TEXT_DY);

  param->label = value_label;
  ui_param_update_label(param);
}
