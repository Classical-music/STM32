#ifndef __BSP_TOUCH_H__
#define __BSP_TOUCH_H__

#include "type.h"

// 扫描方向
typedef enum {
  TOUCH_DIR_VERT, // 竖屏
  TOUCH_DIR_HOR,  // 横屏
} touch_dir_t;

typedef void (*touch_irq_callback_t)(void);

// 触摸屏
typedef struct {
  bool (*init)(void);
  bool (*is_pressed)(void);
  void (*get_xy)(int32_t *x, int32_t *y);
  void (*set_irq_callback)(touch_irq_callback_t callback);
  void (*process)(void);
} touch_t;

extern touch_t touch;

void touch_init(void);
bool touch_is_pressed(void);
void touch_get_xy(int32_t *x, int32_t *y);
void touch_set_irq_callback(touch_irq_callback_t callback);
void touch_process(void);

#endif
