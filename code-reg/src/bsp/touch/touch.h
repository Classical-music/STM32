#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "type.h"

// 扫描方向
typedef enum {
  TOUCH_DIR_VERT, // 竖屏
  TOUCH_DIR_HOR,  // 横屏
} touch_dir_t;

// 触摸屏
typedef struct {
  bool (*init)(void);
  void (*scan_dir)(touch_dir_t);
  void (*adjust)(void); // 触摸屏校准
} touch_t;

extern touch_t touch;

void touch_init(void);

#endif
