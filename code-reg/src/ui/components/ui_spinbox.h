#ifndef __UI_SPINBOX_H__
#define __UI_SPINBOX_H__

#include "lvgl.h"
#include "../common/ui_virtual_keyboard.h"

typedef struct {
  lv_obj_t *container;
  lv_obj_t *spinbox;
  lv_obj_t *minus_button;
  lv_obj_t *plus_button;
  int32_t last_value;
} ui_spinbox_t;

void ui_spinbox_create(ui_spinbox_t *component, lv_obj_t *parent,
                       int32_t min, int32_t max, int32_t current,
                       uint32_t step, uint8_t digit_count);

void ui_spinbox_add_event_cb(ui_spinbox_t *component, lv_event_cb_t event_cb,
                             lv_event_code_t filter, void *user_data);

#endif
