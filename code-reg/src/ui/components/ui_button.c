#include "ui_button.h"

lv_obj_t *ui_button_create(lv_obj_t *parent, const char *text) {
  lv_obj_t *button = lv_btn_create(parent);

  if (text) {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
  }

  return button;
}
