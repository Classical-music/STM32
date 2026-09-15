#ifndef __PAGE_BEEP_H__
#define __PAGE_BEEP_H__

#include "lvgl.h"

/* 创建蜂鸣器控制页面，page 为路由器提供的页面容器。
 * 只在页面首次创建时调用；页面被路由缓存复用时不会再执行。 */
void view_beep_init(lv_obj_t *page, void *params);

/* 页面被路由缓存淘汰、真正销毁前清空本模块的控件引用。 */
void view_beep_destroy(lv_obj_t *page);

#endif
