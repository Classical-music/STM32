#ifndef UI_PARAM_H
#define UI_PARAM_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/* 可调参数步进基础组件。
 * 提供 [− 按钮 / 数值文本 / + 按钮] 一组控件，支持短按走一步、
 * 长按重复按倍率加速，以及跨参数夹紧（on_clamp）与值变化后同步（on_changed）。
 */

typedef struct ui_param ui_param_t;

struct ui_param {
  int32_t value;
  int32_t min;
  int32_t max;
  int32_t step;
  const char *unit;
  /* 显示格式，须为 "%ld%s" 形式（值 + 单位）；NULL 时按 "%ld%s" 处理。 */
  const char *fmt;
  lv_obj_t *label;
  /* 通用范围夹紧后、值变化判定前调用，用于跨参数约束（如呼吸灯差值依赖亮度）。 */
  void (*on_clamp)(ui_param_t *self);
  /* 值变化并提交（label 已刷新）后调用，用于同步硬件。 */
  void (*on_changed)(ui_param_t *self);
};

/* 步进按钮上下文：绑定参数、方向和长按计数。 */
typedef struct {
  ui_param_t *param;
  int8_t dir; /* -1 递减，+1 递增 */
  uint16_t repeats;
} ui_step_btn_t;

/* 刷新参数数值显示。 */
void ui_param_update_label(ui_param_t *param);

/* 创建单个步进按钮（带长按加速），绑定 btn 上下文。 */
lv_obj_t *ui_param_create_step_btn(lv_obj_t *parent, const char *text,
                                   ui_step_btn_t *btn, lv_coord_t x,
                                   lv_coord_t y, lv_coord_t w, lv_coord_t h);

/* 在 parent 内创建 [− 值 +] 横向步进组（固定坐标，parent 宽按 432 设计）。
 * param->label 指向数值控件，minus / plus 自动绑定 param。 */
void ui_param_create_stepper(lv_obj_t *parent, ui_param_t *param,
                             ui_step_btn_t *minus, ui_step_btn_t *plus);

#endif
