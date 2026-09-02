#include "ui_spinbox.h"
#include <stdlib.h>
#include <string.h>

static void ui_spinbox_button_event_cb(lv_event_t *event) {
  ui_spinbox_t *component =
      (ui_spinbox_t *)lv_event_get_user_data(event);
  lv_obj_t *button = lv_event_get_target(event);

  if (button == component->minus_button)
    lv_spinbox_decrement(component->spinbox);
  else
    lv_spinbox_increment(component->spinbox);
}

static void ui_spinbox_add_button_events(lv_obj_t *button,
                                         ui_spinbox_t *component) {
  lv_obj_add_event_cb(button, ui_spinbox_button_event_cb,
                      LV_EVENT_SHORT_CLICKED, component);
  lv_obj_add_event_cb(button, ui_spinbox_button_event_cb,
                      LV_EVENT_LONG_PRESSED_REPEAT, component);
}

static void ui_spinbox_input_event_cb(lv_event_t *event) {
  ui_spinbox_t *component =
      (ui_spinbox_t *)lv_event_get_user_data(event);
  lv_obj_t *spinbox = lv_event_get_target(event);
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_INSERT) {
    char *text = (char *)lv_event_get_param(event);
    if (text != NULL && text[0] == LV_KEY_DEL) {
      uint32_t cursor_pos = lv_textarea_get_cursor_pos(spinbox);
      uint32_t text_length = strlen(lv_textarea_get_text(spinbox));

      if (cursor_pos < text_length)
        lv_textarea_set_cursor_pos(spinbox, cursor_pos + 1);
    }
    return;
  }

  if (code == LV_EVENT_READY) {
    int32_t value = (int32_t)strtol(lv_textarea_get_text(spinbox), NULL, 10);
    if (value != component->last_value) {
      lv_spinbox_set_value(spinbox, value);
      component->last_value = lv_spinbox_get_value(spinbox);
      lv_event_send(spinbox, LV_EVENT_VALUE_CHANGED, NULL);
    }
  }
}

void ui_spinbox_create(ui_spinbox_t *component, lv_obj_t *parent,
                       int32_t min, int32_t max, int32_t current,
                       uint32_t step, uint8_t digit_count) {
  component->container = lv_obj_create(parent);
  lv_obj_set_size(component->container, 170, 34);
  lv_obj_clear_flag(component->container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(component->container, 0, 0);
  lv_obj_set_style_border_width(component->container, 0, 0);
  lv_obj_set_style_bg_opa(component->container, LV_OPA_TRANSP, 0);

  component->spinbox = lv_spinbox_create(component->container);
  lv_spinbox_set_range(component->spinbox, min, max);
  lv_spinbox_set_value(component->spinbox, current);
  lv_spinbox_set_step(component->spinbox, step);
  lv_spinbox_set_digit_format(component->spinbox, digit_count, 0);
  component->last_value = lv_spinbox_get_value(component->spinbox);
  lv_obj_set_size(component->spinbox, 90, 34);
  lv_obj_align(component->spinbox, LV_ALIGN_LEFT_MID, 0, 0);
  ui_virtual_keyboard_bind(component->spinbox,
                           UI_VIRTUAL_KEYBOARD_MODE_NUMBER);
  lv_obj_add_event_cb(component->spinbox, ui_spinbox_input_event_cb,
                      LV_EVENT_INSERT, component);
  lv_obj_add_event_cb(component->spinbox, ui_spinbox_input_event_cb,
                      LV_EVENT_READY, component);

  component->minus_button = lv_btn_create(component->container);
  lv_obj_set_size(component->minus_button, 34, 34);
  lv_obj_align(component->minus_button, LV_ALIGN_LEFT_MID, 94, 0);
  lv_obj_t *minus_label = lv_label_create(component->minus_button);
  lv_label_set_text(minus_label, "-");
  lv_obj_center(minus_label);
  ui_spinbox_add_button_events(component->minus_button, component);

  component->plus_button = lv_btn_create(component->container);
  lv_obj_set_size(component->plus_button, 34, 34);
  lv_obj_align(component->plus_button, LV_ALIGN_LEFT_MID, 136, 0);
  lv_obj_t *plus_label = lv_label_create(component->plus_button);
  lv_label_set_text(plus_label, "+");
  lv_obj_center(plus_label);
  ui_spinbox_add_button_events(component->plus_button, component);
}

void ui_spinbox_add_event_cb(ui_spinbox_t *component, lv_event_cb_t event_cb,
                             lv_event_code_t filter, void *user_data) {
  lv_obj_add_event_cb(component->spinbox, event_cb, filter, user_data);
}
