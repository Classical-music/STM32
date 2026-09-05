#ifndef __UI_SLIDER_H__
#define __UI_SLIDER_H__

#include "lvgl.h"

typedef struct {
  lv_obj_t *container;
  lv_obj_t *slider;
  lv_obj_t *value_label;
  const char *unit;
} ui_slider_t;

void ui_slider_create(ui_slider_t *component, lv_obj_t *parent,
                      int32_t min, int32_t max, int32_t current,
                      const char *unit);

void ui_slider_add_event_cb(ui_slider_t *component, lv_event_cb_t event_cb,
                            lv_event_code_t filter, void *user_data);

int32_t ui_slider_get_value(const ui_slider_t *component);

#endif
