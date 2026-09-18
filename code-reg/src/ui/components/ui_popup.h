#ifndef __UI_POPUP_H__
#define __UI_POPUP_H__

#include "lvgl.h"

/*
 * 通用弹窗组件：半透明遮罩层 + 居中主区域。
 *
 * 组件只提供这层壳子，主区域里放什么由使用方自己画：用 ui_popup_get_body()
 * 拿到主区域对象，在它上面按固定坐标创建控件即可。
 *
 * 组件不额外分配内存，遮罩对象本身就是弹窗句柄，挂在页面上随页面一起销毁，
 * 因此使用方只需保存一个指针，并在页面销毁回调里把它置空。
 */

/*
 * 创建弹窗，创建后默认隐藏，返回遮罩（弹窗句柄），失败返回 NULL。
 *
 * parent 一般是页面容器；遮罩自动铺满 parent 的内容区，无需传坐标。
 * 主区域尺寸 body_width x body_height，在遮罩内水平垂直居中。
 */
lv_obj_t *ui_popup_create(lv_obj_t *parent, lv_coord_t body_width,
                          lv_coord_t body_height);

/* 取弹窗主区域（内容容器），在其上创建弹窗内的控件。 */
lv_obj_t *ui_popup_get_body(lv_obj_t *popup);

/* 显示 / 隐藏弹窗；显示时会把遮罩提到同级最上层。 */
void ui_popup_open(lv_obj_t *popup);
void ui_popup_close(lv_obj_t *popup);

/* 弹窗当前是否可见。popup 为 NULL 时返回 false。 */
bool ui_popup_is_open(lv_obj_t *popup);

/* 设置点击遮罩空白处是否关闭弹窗，默认关闭（创建时即为 true）。 */
void ui_popup_set_dismissible(lv_obj_t *popup, bool dismissible);

#endif
