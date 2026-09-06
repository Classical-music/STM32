#ifndef __UI_COMMON_STYLES_H__
#define __UI_COMMON_STYLES_H__

#include "lvgl.h"

void ui_common_styles_init(void);

lv_style_t *ui_style_form_container(void);
lv_style_t *ui_style_form_row(void);
lv_style_t *ui_style_slider_container(void);
lv_style_t *ui_style_slider_value_label(void);
lv_style_t *ui_style_spinebox_container(void);
lv_style_t *ui_style_spinebox_label(void);

#endif
