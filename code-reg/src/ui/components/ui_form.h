#ifndef __UI_FORM_H__
#define __UI_FORM_H__

#include "lvgl.h"

/* Form 根容器，负责按从上到下的顺序排列多个表单行。 */
typedef struct {
  lv_obj_t *container;
} ui_form_t;

/* 单个表单行，包含右对齐 label；具体控件直接创建到行容器。 */
typedef struct {
  lv_obj_t *container;
  lv_obj_t *label;
} ui_form_row_t;

/* 创建 Form 根容器。 */
void ui_form_create(ui_form_t *form, lv_obj_t *parent);

/* 创建一个表单行，并设置 label 文本。 */
void ui_form_row_create(ui_form_t *form, ui_form_row_t *row,
                        const char *label_text);

/* 将表单控件放置到 label 后方的固定位置。 */
void ui_form_row_set_control(ui_form_row_t *row, lv_obj_t *control);

#endif
