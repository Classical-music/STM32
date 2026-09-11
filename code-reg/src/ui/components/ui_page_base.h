#ifndef __UI_PAGE_BASE_H__
#define __UI_PAGE_BASE_H__

#include "lvgl.h"

/*
 * 在根 screen 上创建一个页面容器。
 *
 * 容器位于系统栏和 Dock 之间，已应用统一的背景色、内边距、文字颜色和纵向
 * Flex 布局，页面只需要往里创建自己的控件。
 *
 * 返回页面容器，创建失败时返回 NULL。
 */
lv_obj_t *ui_page_base_create(lv_obj_t *root);

#endif
