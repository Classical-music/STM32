#include "ui_form.h"
#include "task.h"
#include <stdio.h>

/* 输出 Form 阶段耗时；所有时间点在打印前完成采样。 */
static void ui_form_print_timing(const char *type,
                                 TickType_t start_tick,
                                 TickType_t container_tick,
                                 TickType_t layout_tick,
                                 TickType_t label_tick,
                                 TickType_t end_tick) {
  printf("[ui_form] %s container=%lu layout=%lu label=%lu "
         "finalize=%lu total=%lu ms\r\n",
         type,
         (unsigned long)((container_tick - start_tick) * portTICK_PERIOD_MS),
         (unsigned long)((layout_tick - container_tick) * portTICK_PERIOD_MS),
         (unsigned long)((label_tick - layout_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - label_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/* 左侧 label 的固定宽度；文字超出后由 LVGL 自动换行。 */
#define UI_FORM_LABEL_WIDTH 64
/* label 与输入控件之间的水平间距。 */
#define UI_FORM_LABEL_CONTROL_GAP 32
#define UI_FORM_CONTROL_POS (UI_FORM_LABEL_WIDTH + UI_FORM_LABEL_CONTROL_GAP)

/* Form 中相邻两行之间的垂直间距。 */
#define UI_FORM_ROW_GAP 12

/* 创建 Form 根容器，并配置为纵向 Flex 布局。 */
void ui_form_create(ui_form_t *form, lv_obj_t *parent) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t container_tick;
  TickType_t layout_tick;
  TickType_t label_tick;
  TickType_t end_tick;

  /* Form 本身只负责排列多个 form row，不直接承载输入控件。 */
  form->container = lv_obj_create(parent);
  container_tick = xTaskGetTickCount();
  lv_obj_set_width(form->container, LV_PCT(100));
  lv_obj_set_height(form->container, LV_SIZE_CONTENT);
  layout_tick = xTaskGetTickCount();

  /* 子项从上到下排列，行与行之间使用固定间距。 */
  lv_obj_set_layout(form->container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(form->container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(form->container, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_all(form->container, 0, 0);
  lv_obj_set_style_pad_row(form->container, UI_FORM_ROW_GAP, 0);
  label_tick = xTaskGetTickCount();

  /* Form 是透明布局容器，不显示边框、背景，也不需要滚动。 */
  lv_obj_set_style_border_width(form->container, 0, 0);
  lv_obj_set_style_bg_opa(form->container, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(form->container, LV_OBJ_FLAG_SCROLLABLE);
  end_tick = xTaskGetTickCount();

  ui_form_print_timing("form", start_tick, container_tick, layout_tick,
                      label_tick, end_tick);
}

/* 创建一行表单：左侧 label，右侧输入控件容器。 */
void ui_form_row_create(ui_form_t *form, ui_form_row_t *row,
                        const char *label_text) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t container_tick;
  TickType_t layout_tick;
  TickType_t label_tick;
  TickType_t end_tick;

  /* 创建当前行，不使用布局；控件位置由 ui_form_row_set_control() 指定。 */
  row->container = lv_obj_create(form->container);
  container_tick = xTaskGetTickCount();
  lv_obj_set_width(row->container, LV_PCT(100));
  lv_obj_set_height(row->container, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(row->container, 0, 0);
  layout_tick = xTaskGetTickCount();

  /* 行本身不显示背景和边框，也不滚动；允许子控件超出边界显示。 */
  lv_obj_set_style_border_width(row->container, 0, 0);
  lv_obj_set_style_bg_opa(row->container, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(row->container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(row->container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

  /* 创建左侧 label；先完成显示约束，再写入文本，避免文本设置后重复计算布局。 */
  row->label = lv_label_create(row->container);
  lv_obj_set_width(row->label, UI_FORM_LABEL_WIDTH);
  lv_label_set_long_mode(row->label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(row->label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_label_set_text(row->label, label_text);
  label_tick = xTaskGetTickCount();

  end_tick = xTaskGetTickCount();

  ui_form_print_timing("row", start_tick, container_tick, layout_tick,
                      label_tick, end_tick);
}

/* 将控件定位到 Label 右侧固定间距处，并在行内垂直居中。 */
void ui_form_row_set_control(ui_form_row_t *row, lv_obj_t *control) {
  (void)row;
  lv_obj_align(control, LV_ALIGN_LEFT_MID, UI_FORM_CONTROL_POS, 0);
}
