#ifndef __UI_VIRTUAL_KEYBOARD_H__
#define __UI_VIRTUAL_KEYBOARD_H__

#include "lvgl.h"

typedef enum {
  UI_VIRTUAL_KEYBOARD_MODE_NUMBER,
} ui_virtual_keyboard_mode_t;

void ui_virtual_keyboard_init_async(void);
void ui_virtual_keyboard_open(ui_virtual_keyboard_mode_t mode);
void ui_virtual_keyboard_close(void);
void ui_virtual_keyboard_show_input(lv_obj_t *reference);
void ui_virtual_keyboard_hide_input(void);
void ui_virtual_keyboard_configure_input(int32_t min,
                                         int32_t max,
                                         int32_t value,
                                         uint32_t step,
                                         uint8_t digit_count);
int32_t ui_virtual_keyboard_get_input_value(void);
void ui_virtual_keyboard_set_input_event_cb(lv_event_cb_t event_cb,
                                            void *user_data);
void *ui_virtual_keyboard_get_input_event_user_data(void);

#endif
