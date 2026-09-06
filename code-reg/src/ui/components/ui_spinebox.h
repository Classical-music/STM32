#ifndef __UI_SPINEBOX_H__
#define __UI_SPINEBOX_H__

#include "lvgl.h"
#include "../common/ui_virtual_keyboard.h"

/* 带减号按钮、数字输入区和加号按钮的公共 spinebox 组件。 */
typedef struct {
  /* 组件根容器，负责承载按钮和数字输入区。 */
  lv_obj_t *container;
  /* 显示当前数值的轻量级 label 控件。 */
  lv_obj_t *spinebox;
  /* 左侧减号按钮。 */
  lv_obj_t *minus_button;
  /* 右侧加号按钮。 */
  lv_obj_t *plus_button;
  /* 当前数值及其有效范围和步长。 */
  int32_t value;
  int32_t min;
  int32_t max;
  uint32_t step;
  uint8_t digit_count;
  char value_text[16];
  /* 打开虚拟键盘前的数值，用于取消输入时恢复。 */
  int32_t keyboard_original_value;
} ui_spinebox_t;

/* 创建 spinebox，并设置范围、初始值、步长和显示位数。 */
void ui_spinebox_create(ui_spinebox_t *component, lv_obj_t *parent,
                       int32_t min, int32_t max, int32_t current,
                       uint32_t step, uint8_t digit_count);

/* 为内部数字 label 注册外部事件回调。 */
void ui_spinebox_add_event_cb(ui_spinebox_t *component, lv_event_cb_t event_cb,
                             lv_event_code_t filter, void *user_data);

/* 获取当前数值。 */
int32_t ui_spinebox_get_value(const ui_spinebox_t *component);

#endif
