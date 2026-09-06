#include "ui_slider.h"
#include "ui_common_styles.h"
#include "task.h"
#include <stdio.h>

/* 输出 slider 各阶段耗时；所有时间点在打印前完成采样。 */
static void ui_slider_print_timing(TickType_t start_tick,
                                   TickType_t container_tick,
                                   TickType_t slider_tick,
                                   TickType_t label_tick,
                                   TickType_t event_tick,
                                   TickType_t end_tick) {
  printf("[ui_slider] container=%lu slider=%lu label=%lu event=%lu "
         "finalize=%lu total=%lu ms\r\n",
         (unsigned long)((container_tick - start_tick) * portTICK_PERIOD_MS),
         (unsigned long)((slider_tick - container_tick) * portTICK_PERIOD_MS),
         (unsigned long)((label_tick - slider_tick) * portTICK_PERIOD_MS),
         (unsigned long)((event_tick - label_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - event_tick) * portTICK_PERIOD_MS),
         (unsigned long)((end_tick - start_tick) * portTICK_PERIOD_MS));
}

/* slider 交互区域的宽度，不包含右侧数值标签。 */
#define UI_SLIDER_CONTROL_WIDTH 128

/* 根据 slider 当前值刷新右侧的数值和单位文本。 */
static void ui_slider_update_value_label(ui_slider_t *component) {
  snprintf(component->value_text, sizeof(component->value_text), "%ld %s",
           (long)lv_slider_get_value(component->slider), component->unit);
  lv_label_set_text_static(component->value_label, component->value_text);
}

/* slider 数值变化事件：先更新右侧标签，再由外部回调处理业务逻辑。 */
static void ui_slider_value_event_cb(lv_event_t *event) {
  ui_slider_t *component =
      (ui_slider_t *)lv_event_get_user_data(event);
  ui_slider_update_value_label(component);
}

/* 创建 slider 组件及其右侧数值标签。 */
void ui_slider_create(ui_slider_t *component, lv_obj_t *parent,
                      int32_t min, int32_t max, int32_t current,
                      const char *unit) {
  TickType_t start_tick = xTaskGetTickCount();
  TickType_t container_tick;
  TickType_t slider_tick;
  TickType_t label_tick;
  TickType_t event_tick;
  TickType_t end_tick;

  /* 创建根容器，统一承载 slider 和右侧数值标签。 */
  ui_common_styles_init();
  component->container = lv_obj_create(parent);
  lv_obj_add_style(component->container, ui_style_slider_container(), 0);
  lv_obj_set_size(component->container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  /* 容器只负责组合布局，不允许滚动，也不显示自身边框和背景。 */
  lv_obj_clear_flag(component->container, LV_OBJ_FLAG_SCROLLABLE);
  /* 允许内部元素超出容器范围后仍可见。 */
  lv_obj_add_flag(component->container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  /* 容器使用水平 Flex 布局，让 slider 和数值标签从左到右排列。 */
  lv_obj_set_layout(component->container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(component->container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(component->container, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  container_tick = xTaskGetTickCount();

  /* 创建并初始化 slider 的取值范围和当前值。 */
  component->slider = lv_slider_create(component->container);
  lv_slider_set_range(component->slider, min, max);
  lv_slider_set_value(component->slider, current, LV_ANIM_OFF);
  /* slider 宽度固定，作为水平布局中的第一个子控件。 */
  lv_obj_set_width(component->slider, UI_SLIDER_CONTROL_WIDTH);
  slider_tick = xTaskGetTickCount();

  /* 创建右侧数值标签，并在水平布局中自动排列。 */
  component->unit = unit;
  snprintf(component->value_text, sizeof(component->value_text), "%ld %s",
           (long)current, component->unit);
  component->value_label = lv_label_create(component->container);
  lv_label_set_text_static(component->value_label, component->value_text);
  lv_obj_add_style(component->value_label, ui_style_slider_value_label(), 0);
  label_tick = xTaskGetTickCount();

  /* 监听 slider 数值变化，自动刷新数值标签。 */
  lv_obj_add_event_cb(component->slider, ui_slider_value_event_cb,
                      LV_EVENT_VALUE_CHANGED, component);
  event_tick = xTaskGetTickCount();
  end_tick = xTaskGetTickCount();

  ui_slider_print_timing(start_tick, container_tick, slider_tick,
                         label_tick, event_tick, end_tick);
}

/* 注册外部事件回调，不影响组件内部的数值标签刷新。 */
void ui_slider_add_event_cb(ui_slider_t *component, lv_event_cb_t event_cb,
                            lv_event_code_t filter, void *user_data) {
  lv_obj_add_event_cb(component->slider, event_cb, filter, user_data);
}

/* 获取 slider 当前值，供页面读取业务参数。 */
int32_t ui_slider_get_value(const ui_slider_t *component) {
  return lv_slider_get_value(component->slider);
}
