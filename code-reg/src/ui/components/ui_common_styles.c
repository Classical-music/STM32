#include "ui_common_styles.h"

#define UI_COMMON_FORM_ROW_GAP 12

static lv_style_t form_container_style;
static lv_style_t form_row_style;
static lv_style_t slider_container_style;
static lv_style_t slider_value_label_style;
static lv_style_t spinebox_container_style;
static lv_style_t spinebox_label_style;
static bool styles_initialized;

void ui_common_styles_init(void) {
  if (styles_initialized)
    return;

  lv_style_init(&form_container_style);
  lv_style_set_pad_all(&form_container_style, 0);
  lv_style_set_pad_row(&form_container_style, UI_COMMON_FORM_ROW_GAP);
  lv_style_set_border_width(&form_container_style, 0);
  lv_style_set_bg_opa(&form_container_style, LV_OPA_TRANSP);

  lv_style_init(&form_row_style);
  lv_style_set_pad_all(&form_row_style, 0);
  lv_style_set_border_width(&form_row_style, 0);
  lv_style_set_bg_opa(&form_row_style, LV_OPA_TRANSP);

  lv_style_init(&slider_container_style);
  lv_style_set_pad_all(&slider_container_style, 0);
  lv_style_set_border_width(&slider_container_style, 0);
  lv_style_set_bg_opa(&slider_container_style, LV_OPA_TRANSP);
  lv_style_set_pad_column(&slider_container_style, 24);

  lv_style_init(&slider_value_label_style);
  lv_style_set_text_align(&slider_value_label_style, LV_TEXT_ALIGN_RIGHT);

  lv_style_init(&spinebox_container_style);
  lv_style_set_pad_all(&spinebox_container_style, 0);
  lv_style_set_radius(&spinebox_container_style, 8);
  lv_style_set_border_width(&spinebox_container_style, 1);
  lv_style_set_bg_opa(&spinebox_container_style, LV_OPA_TRANSP);

  lv_style_init(&spinebox_label_style);
  lv_style_set_text_align(&spinebox_label_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_pad_top(&spinebox_label_style, 10);

  styles_initialized = true;
}

#define STYLE_ACCESSOR(name, style) \
  lv_style_t *name(void) {          \
    return &style;                  \
  }

STYLE_ACCESSOR(ui_style_form_container, form_container_style)
STYLE_ACCESSOR(ui_style_form_row, form_row_style)
STYLE_ACCESSOR(ui_style_slider_container, slider_container_style)
STYLE_ACCESSOR(ui_style_slider_value_label, slider_value_label_style)
STYLE_ACCESSOR(ui_style_spinebox_container, spinebox_container_style)
STYLE_ACCESSOR(ui_style_spinebox_label, spinebox_label_style)
