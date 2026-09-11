#ifndef __DOCK_H__
#define __DOCK_H__

#include "lvgl.h"

/* Dock 栏固定高度，页面需要为该高度预留底部空间。 */
#define DOCK_HEIGHT 60

/* 在默认屏幕（lv_scr_act()）上创建底部 Dock，GUI 初始化阶段调用一次。 */
void dock_init(void);

#endif
