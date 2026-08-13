// system_bar.h
// 跨页面的顶部系统栏（显示时间、电量等）

#ifndef __SYSTEM_BAR_H__
#define __SYSTEM_BAR_H__

#include "lvgl.h"

// 初始化系统栏（在 GUI 初始化阶段调用一次）
void system_bar_init(void);

// 系统栏内部使用系统时钟更新时间（HH:MM）
// 外部不需要也不提供设置时间的接口

// 显示/隐藏系统栏
void system_bar_show(void);
void system_bar_hide(void);

#endif
