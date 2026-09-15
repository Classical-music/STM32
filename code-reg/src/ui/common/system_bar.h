// system_bar.h
// 跨页面的顶部系统栏（左侧显示页面标题，右侧显示时间）

#ifndef __SYSTEM_BAR_H__
#define __SYSTEM_BAR_H__

#include "lvgl.h"

/* 系统栏固定高度，页面需要为该高度预留顶部空间。 */
#define SYSTEM_BAR_HEIGHT 56

/* 页面标题最大长度（不含结尾 '\0'）。 */
#define SYSTEM_BAR_TITLE_MAX_LEN 32

// 在默认屏幕（lv_scr_act()）上创建系统栏，GUI 初始化阶段调用一次
void system_bar_init(void);

// 设置系统栏左侧显示的页面标题，传 NULL 或空字符串清空标题。
// 初始化之前调用也可以，标题会暂存并在初始化后显示。
void system_bar_set_title(const char *title);

// 系统栏内部使用系统时钟更新时间（HH:MM）
// 外部不需要也不提供设置时间的接口

// 显示/隐藏系统栏
void system_bar_show(void);
void system_bar_hide(void);

#endif
