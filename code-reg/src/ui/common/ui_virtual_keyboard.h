#ifndef __UI_VIRTUAL_KEYBOARD_H__
#define __UI_VIRTUAL_KEYBOARD_H__

#include "lvgl.h"

typedef enum {
  UI_VIRTUAL_KEYBOARD_MODE_NUMBER,
} ui_virtual_keyboard_mode_t;

void ui_virtual_keyboard_bind(lv_obj_t *target,
                              ui_virtual_keyboard_mode_t mode);
void ui_virtual_keyboard_open(lv_obj_t *target,
                              ui_virtual_keyboard_mode_t mode);
void ui_virtual_keyboard_close(void);
void ui_virtual_keyboard_add_event_cb(lv_event_cb_t event_cb,
                                      lv_event_code_t filter,
                                      void *user_data);

#endif
