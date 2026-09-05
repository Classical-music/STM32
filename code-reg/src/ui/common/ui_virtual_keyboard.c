#include "ui_virtual_keyboard.h"
#include <string.h>

#define UI_VIRTUAL_KEYBOARD_HEIGHT 220
#define UI_VIRTUAL_KEYBOARD_BOTTOM_RESERVED 44

static lv_obj_t *g_virtual_keyboard;
static lv_obj_t *g_virtual_keyboard_input;
static lv_event_cb_t g_input_event_cb = NULL;
static void *g_input_event_user_data;

static void ui_virtual_keyboard_close_event_cb(lv_event_t *event);
static void ui_virtual_keyboard_input_event_cb(lv_event_t *event);

/* 创建全局共享的隐藏输入框和虚拟键盘。 */
static void ui_virtual_keyboard_ensure_created(void) {
  if (g_virtual_keyboard_input == NULL) {
    g_virtual_keyboard_input = lv_spinbox_create(lv_layer_top());
    lv_obj_add_flag(g_virtual_keyboard_input, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(g_virtual_keyboard_input,
                                LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(g_virtual_keyboard_input, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_virtual_keyboard_input, 0, 0);
    lv_obj_add_event_cb(g_virtual_keyboard_input,
                        ui_virtual_keyboard_input_event_cb,
                        LV_EVENT_INSERT, NULL);
  }

  if (g_virtual_keyboard == NULL) {
    g_virtual_keyboard = lv_keyboard_create(lv_layer_top());
    lv_obj_add_event_cb(g_virtual_keyboard,
                        ui_virtual_keyboard_close_event_cb,
                        LV_EVENT_CANCEL, NULL);
    lv_obj_add_event_cb(g_virtual_keyboard,
                        ui_virtual_keyboard_close_event_cb,
                        LV_EVENT_READY, NULL);
    lv_obj_add_flag(g_virtual_keyboard, LV_OBJ_FLAG_HIDDEN);
  }
}

/* 调整删除键行为，使删除作用于当前光标选中的数字。 */
static void ui_virtual_keyboard_input_event_cb(lv_event_t *event) {
  char *text = (char *)lv_event_get_param(event);
  uint32_t cursor_pos;
  size_t text_length;

  if (text == NULL || text[0] != LV_KEY_DEL)
    return;

  cursor_pos = lv_textarea_get_cursor_pos(g_virtual_keyboard_input);
  text_length = strlen(lv_textarea_get_text(g_virtual_keyboard_input));

  if (cursor_pos < text_length)
    lv_textarea_set_cursor_pos(g_virtual_keyboard_input,
                               cursor_pos + 1);
}

/* 异步创建全局共享输入框和虚拟键盘。 */
static void ui_virtual_keyboard_init_async_cb(void *user_data) {
  (void)user_data;
  ui_virtual_keyboard_ensure_created();
}

/* 在 GUI 空闲时预创建全局共享输入框和虚拟键盘。 */
void ui_virtual_keyboard_init_async(void) {
  lv_async_call(ui_virtual_keyboard_init_async_cb, NULL);
}

/* 将键盘固定在底部 Dock 上方。 */
static void ui_virtual_keyboard_layout(void) {
  lv_obj_set_width(g_virtual_keyboard, lv_disp_get_hor_res(NULL));
  lv_obj_set_height(g_virtual_keyboard, UI_VIRTUAL_KEYBOARD_HEIGHT);
  lv_obj_align(g_virtual_keyboard, LV_ALIGN_BOTTOM_MID, 0,
               -UI_VIRTUAL_KEYBOARD_BOTTOM_RESERVED);
}

/* 键盘完成确认或取消输入后，隐藏键盘并通知当前输入组件。 */
static void ui_virtual_keyboard_close_event_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code != LV_EVENT_CANCEL && code != LV_EVENT_READY)
    return;

  lv_obj_add_flag(g_virtual_keyboard, LV_OBJ_FLAG_HIDDEN);

  if (g_input_event_cb != NULL) {
    lv_event_cb_t input_event_cb = g_input_event_cb;
    input_event_cb(event);
    g_input_event_cb = NULL;
    g_input_event_user_data = NULL;
  }
}

/* 打开并复用全局共享 spinbox 和虚拟键盘。 */
void ui_virtual_keyboard_open(ui_virtual_keyboard_mode_t mode) {
  ui_virtual_keyboard_ensure_created();

  lv_keyboard_set_mode(g_virtual_keyboard,
                       (lv_keyboard_mode_t)mode + LV_KEYBOARD_MODE_NUMBER);
  lv_keyboard_set_textarea(g_virtual_keyboard, g_virtual_keyboard_input);
  ui_virtual_keyboard_layout();
  lv_obj_clear_flag(g_virtual_keyboard, LV_OBJ_FLAG_HIDDEN);
}

/* 隐藏全局虚拟键盘，并清除当前输入组件上下文。 */
void ui_virtual_keyboard_close(void) {
  if (g_virtual_keyboard != NULL)
    lv_obj_add_flag(g_virtual_keyboard, LV_OBJ_FLAG_HIDDEN);

  g_input_event_cb = NULL;
  g_input_event_user_data = NULL;
}

/* 显示共享 spinbox，并覆盖到指定 Label 的位置。 */
void ui_virtual_keyboard_show_input(lv_obj_t *reference) {
  lv_area_t reference_coords;
  lv_area_t top_coords;
  lv_coord_t width;
  lv_coord_t height;

  ui_virtual_keyboard_ensure_created();
  lv_obj_get_coords(reference, &reference_coords);
  lv_obj_get_coords(lv_layer_top(), &top_coords);

  width = reference_coords.x2 - reference_coords.x1 + 1;
  height = reference_coords.y2 - reference_coords.y1 + 1;

  lv_obj_set_pos(g_virtual_keyboard_input,
                 reference_coords.x1 - top_coords.x1,
                 reference_coords.y1 - top_coords.y1);
  lv_obj_set_size(g_virtual_keyboard_input, width, height);
  lv_obj_clear_flag(g_virtual_keyboard_input, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(g_virtual_keyboard_input);
}

/* 隐藏共享 spinbox。 */
void ui_virtual_keyboard_hide_input(void) {
  if (g_virtual_keyboard_input != NULL)
    lv_obj_add_flag(g_virtual_keyboard_input, LV_OBJ_FLAG_HIDDEN);
}

/* 配置共享 spinbox 的范围、步长、位数和当前值。 */
void ui_virtual_keyboard_configure_input(int32_t min,
                                         int32_t max,
                                         int32_t value,
                                         uint32_t step,
                                         uint8_t digit_count) {
  ui_virtual_keyboard_ensure_created();
  lv_spinbox_set_range(g_virtual_keyboard_input, min, max);
  lv_spinbox_set_step(g_virtual_keyboard_input, step);
  lv_spinbox_set_digit_format(g_virtual_keyboard_input,
                              digit_count, 0);
  lv_spinbox_set_value(g_virtual_keyboard_input, value);
}

/* 获取共享 spinbox 当前的输入值。 */
int32_t ui_virtual_keyboard_get_input_value(void) {
  if (g_virtual_keyboard_input == NULL)
    return 0;

  return lv_spinbox_get_value(g_virtual_keyboard_input);
}

/* 设置确认或取消事件的接收回调及其上下文。 */
void ui_virtual_keyboard_set_input_event_cb(lv_event_cb_t event_cb,
                                            void *user_data) {
  g_input_event_cb = event_cb;
  g_input_event_user_data = user_data;
}

/* 获取当前输入回调绑定的组件上下文。 */
void *ui_virtual_keyboard_get_input_event_user_data(void) {
  return g_input_event_user_data;
}
