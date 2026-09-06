#include "ui_spinebox.h"
#include "ui_button.h"
#include "ui_common_styles.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 输出轻量 spinebox 创建阶段的耗时；所有时间点均在打印前完成采样。 */
static void ui_spinebox_print_timing(TickType_t start_tick,
                                    TickType_t container_tick,
                                    TickType_t label_tick,
                                    TickType_t minus_tick,
                                    TickType_t plus_tick,
                                    TickType_t end_tick) {
  printf("[ui_spinebox] container=%lu label=%lu minus_button=%lu "
         "plus_button=%lu finalize=%lu total=%lu ms\r\n",
         (unsigned long)((container_tick - start_tick) * portTICK_PERIOD_MS),
         (unsigned long)((label_tick - container_tick) * portTICK_PERIOD_MS),
         (unsigned long)((minus_tick - label_tick) * portTICK_PERIOD_MS),
         (unsigned long)((plus_tick - minus_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - plus_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/* 将数值限制在组件允许的最小值和最大值之间。 */
static int32_t ui_spinebox_clamp_value(const ui_spinebox_t *component,
                                      int32_t value) {
  if (value < component->min)
    return component->min;
  if (value > component->max)
    return component->max;
  return value;
}

/* 刷新数字 label 的显示文本。 */
static void ui_spinebox_update_label(ui_spinebox_t *component) {
  snprintf(component->value_text, sizeof(component->value_text), "%0*ld",
           component->digit_count, (long)component->value);
  lv_label_set_text_static(component->spinebox, component->value_text);
}

/* 发送数值变化事件，事件目标为对外暴露的数字 label。 */
static void ui_spinebox_send_value_changed(ui_spinebox_t *component) {
  lv_event_send(component->spinebox, LV_EVENT_VALUE_CHANGED, NULL);
}

/* 处理减号和加号按钮的点击、长按重复事件。 */
static void ui_spinebox_button_event_cb(lv_event_t *event) {
  ui_spinebox_t *component =
      (ui_spinebox_t *)lv_event_get_user_data(event);
  lv_obj_t *button = lv_event_get_current_target(event);
  int32_t old_value = component->value;

  if (button != component->minus_button &&
      button != component->plus_button)
    return;

  if (button == component->minus_button) {
    if (component->value >= component->min + (int32_t)component->step)
      component->value -= (int32_t)component->step;
    else
      component->value = component->min;
  } else {
    if (component->value <= component->max - (int32_t)component->step)
      component->value += (int32_t)component->step;
    else
      component->value = component->max;
  }

  if (component->value != old_value) {
    ui_spinebox_update_label(component);
    ui_spinebox_send_value_changed(component);
  }
}

/* 为单个按钮注册短按和长按重复事件。 */
static void ui_spinebox_add_button_events(lv_obj_t *button,
                                         ui_spinebox_t *component) {
  lv_obj_add_event_cb(button, ui_spinebox_button_event_cb,
                      LV_EVENT_SHORT_CLICKED, component);
  lv_obj_add_event_cb(button, ui_spinebox_button_event_cb,
                      LV_EVENT_LONG_PRESSED_REPEAT, component);
}

static void ui_spinebox_input_event_cb(lv_event_t *event);

/* 数字 label 被点击时，准备全局共享输入框并打开数字键盘。 */
static void ui_spinebox_label_event_cb(lv_event_t *event) {
  ui_spinebox_t *component =
      (ui_spinebox_t *)lv_event_get_user_data(event);
  lv_event_code_t code = lv_event_get_code(event);

  if (code != LV_EVENT_CLICKED && code != LV_EVENT_FOCUSED)
    return;

  component->keyboard_original_value = component->value;

  ui_virtual_keyboard_set_input_event_cb(ui_spinebox_input_event_cb,
                                          component);
  ui_virtual_keyboard_configure_input(component->min,
                                      component->max,
                                      component->value,
                                      component->step,
                                      component->digit_count);
  ui_virtual_keyboard_show_input(component->spinebox);
  lv_obj_add_flag(component->spinebox, LV_OBJ_FLAG_HIDDEN);
  ui_virtual_keyboard_open(UI_VIRTUAL_KEYBOARD_MODE_NUMBER);
}

/* 处理虚拟键盘确认和取消事件。 */
static void ui_spinebox_input_event_cb(lv_event_t *event) {
  ui_spinebox_t *component =
      (ui_spinebox_t *)ui_virtual_keyboard_get_input_event_user_data();
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_VALUE_CHANGED)
    return;

  lv_obj_clear_flag(component->spinebox, LV_OBJ_FLAG_HIDDEN);
  ui_virtual_keyboard_hide_input();

  if (code == LV_EVENT_CANCEL) {
    component->value = component->keyboard_original_value;
    ui_spinebox_update_label(component);
    return;
  }

  if (code == LV_EVENT_READY) {
    int32_t old_value = component->keyboard_original_value;
    int32_t value = (int32_t)ui_virtual_keyboard_get_input_value();
    component->value = ui_spinebox_clamp_value(component, value);
    ui_spinebox_update_label(component);

    if (component->value != old_value)
      ui_spinebox_send_value_changed(component);
  }
}

/* 创建使用 label 显示数值的轻量 spinebox 组件。 */
void ui_spinebox_create(ui_spinebox_t *component, lv_obj_t *parent,
                       int32_t min, int32_t max, int32_t current,
                       uint32_t step, uint8_t digit_count) {
  TickType_t start_tick;
  TickType_t container_tick;
  TickType_t label_tick;
  TickType_t minus_tick;
  TickType_t plus_tick;
  TickType_t end_tick;

  component->min = min;
  component->max = max;
  component->step = step;
  component->value = ui_spinebox_clamp_value(component, current);
  component->digit_count = digit_count;
  component->keyboard_original_value = component->value;
  ui_common_styles_init();
  start_tick = xTaskGetTickCount();

  /* 创建组件根容器，统一承载按钮和数字 label。 */
  component->container = lv_obj_create(parent);
  lv_obj_add_style(component->container, ui_style_spinebox_container(), 0);
  lv_obj_set_size(component->container, 176, 40);
  lv_obj_clear_flag(component->container, LV_OBJ_FLAG_SCROLLABLE);
  container_tick = xTaskGetTickCount();

  /* 创建数字显示 label，并立即绑定稳定的初始文本，避免先处理默认文本。 */
  snprintf(component->value_text, sizeof(component->value_text), "%0*ld",
           component->digit_count, (long)component->value);
  component->spinebox = lv_label_create(component->container);
  lv_obj_set_size(component->spinebox, 90, 40);
  lv_obj_set_pos(component->spinebox, 43, 0);
  lv_obj_add_style(component->spinebox, ui_style_spinebox_label(), 0);
  lv_label_set_text_static(component->spinebox, component->value_text);
  lv_obj_add_flag(component->spinebox, LV_OBJ_FLAG_CLICKABLE);

  /* 注册数字 label 的点击和焦点事件；共享输入框在打开键盘时才创建。 */
  lv_obj_add_event_cb(component->spinebox, ui_spinebox_label_event_cb,
                      LV_EVENT_CLICKED, component);
  lv_obj_add_event_cb(component->spinebox, ui_spinebox_label_event_cb,
                      LV_EVENT_FOCUSED, component);
  label_tick = xTaskGetTickCount();

  /* 创建左侧减号按钮，并注册短按和长按快速递减。 */
  component->minus_button = ui_button_create(component->container, "-");
  lv_obj_set_size(component->minus_button, 38, 38);
  lv_obj_set_pos(component->minus_button, 0, 1);
  ui_spinebox_add_button_events(component->minus_button, component);
  minus_tick = xTaskGetTickCount();

  /* 创建右侧加号按钮，并注册短按和长按快速递增。 */
  component->plus_button = ui_button_create(component->container, "+");
  lv_obj_set_size(component->plus_button, 38, 38);
  lv_obj_set_pos(component->plus_button, 138, 1);
  ui_spinebox_add_button_events(component->plus_button, component);
  plus_tick = xTaskGetTickCount();
  end_tick = xTaskGetTickCount();

  ui_spinebox_print_timing(start_tick, container_tick, label_tick,
                          minus_tick, plus_tick, end_tick);
}

/* 向外部注册数字 label 的事件回调，供页面监听数值变化。 */
void ui_spinebox_add_event_cb(ui_spinebox_t *component, lv_event_cb_t event_cb,
                             lv_event_code_t filter, void *user_data) {
  lv_obj_add_event_cb(component->spinebox, event_cb, filter, user_data);
}

/* 获取当前数值。 */
int32_t ui_spinebox_get_value(const ui_spinebox_t *component) {
  return component->value;
}
