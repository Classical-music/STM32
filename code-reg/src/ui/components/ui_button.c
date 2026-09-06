#include "ui_button.h"

lv_obj_t *ui_button_create(lv_obj_t *parent, const char *text) {
  lv_obj_t *button = lv_btn_create(parent);

  if (text) {
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text_static(label, text);
    lv_obj_center(label);
  }

  return button;
}
