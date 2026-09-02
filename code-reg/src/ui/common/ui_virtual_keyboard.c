#include "ui_virtual_keyboard.h"

#define UI_VIRTUAL_KEYBOARD_HEIGHT 220
#define UI_VIRTUAL_KEYBOARD_BOTTOM_RESERVED 44

static lv_obj_t *g_virtual_keyboard;
static ui_virtual_keyboard_mode_t g_virtual_keyboard_mode;

static void ui_virtual_keyboard_layout(void) {
  lv_obj_set_width(g_virtual_keyboard, lv_disp_get_hor_res(NULL));
  lv_obj_set_height(g_virtual_keyboard, UI_VIRTUAL_KEYBOARD_HEIGHT);
  lv_obj_align(g_virtual_keyboard, LV_ALIGN_BOTTOM_MID, 0,
               -(UI_VIRTUAL_KEYBOARD_BOTTOM_RESERVED));
}

static void ui_virtual_keyboard_close_event_cb(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CANCEL ||
      lv_event_get_code(event) == LV_EVENT_READY)
    ui_virtual_keyboard_close();
}

static void ui_virtual_keyboard_target_event_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  /* 点击或获取焦点时，都将当前输入控件交给全局键盘。 */
  if (code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED)
    ui_virtual_keyboard_open(lv_event_get_target(event), g_virtual_keyboard_mode);
}

void ui_virtual_keyboard_bind(lv_obj_t *target,
                              ui_virtual_keyboard_mode_t mode) {
  g_virtual_keyboard_mode = mode;
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

  lv_keyboard_set_mode(g_virtual_keyboard, (lv_keyboard_mode_t)mode +
                                                   LV_KEYBOARD_MODE_NUMBER);
  lv_keyboard_set_textarea(g_virtual_keyboard, target);
  lv_obj_add_event_cb(target, ui_virtual_keyboard_target_event_cb,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(target, ui_virtual_keyboard_target_event_cb,
                      LV_EVENT_FOCUSED, NULL);
}

/* 隐藏键盘并解除当前输入目标。 */
void ui_virtual_keyboard_close(void) {
  if (g_virtual_keyboard == NULL)
    return;

  lv_obj_add_flag(g_virtual_keyboard, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_textarea(g_virtual_keyboard, NULL);
}

void ui_virtual_keyboard_open(lv_obj_t *target, ui_virtual_keyboard_mode_t mode) {
  if (g_virtual_keyboard == NULL) {
    g_virtual_keyboard = lv_keyboard_create(lv_layer_top());
    lv_obj_add_event_cb(g_virtual_keyboard,
                        ui_virtual_keyboard_close_event_cb,
                        LV_EVENT_CANCEL, NULL);
    lv_obj_add_event_cb(g_virtual_keyboard,
                        ui_virtual_keyboard_close_event_cb,
                        LV_EVENT_READY, NULL);
  } 

  g_virtual_keyboard_mode = mode;
  lv_keyboard_set_mode(g_virtual_keyboard, (lv_keyboard_mode_t)mode + LV_KEYBOARD_MODE_NUMBER);
  lv_keyboard_set_textarea(g_virtual_keyboard, target);
  ui_virtual_keyboard_layout();
  lv_obj_clear_flag(g_virtual_keyboard, LV_OBJ_FLAG_HIDDEN);
}

/* 为全局虚拟键盘注册额外事件回调。 */
void ui_virtual_keyboard_add_event_cb(lv_event_cb_t event_cb,
                                      lv_event_code_t filter,
                                      void *user_data) {
  if (g_virtual_keyboard == NULL)
    return;

  lv_obj_add_event_cb(g_virtual_keyboard, event_cb, filter, user_data);
}
