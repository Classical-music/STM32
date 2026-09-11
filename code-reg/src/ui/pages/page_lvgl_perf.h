#ifndef __PAGE_LVGL_PERF_H__
#define __PAGE_LVGL_PERF_H__

#include "lvgl.h"

/* 创建控件创建耗时测试页面，page 为路由器提供的页面容器。 */
void view_lvgl_perf_init(lv_obj_t *page, void *params);
/* 页面被路由缓存复用时清空上一轮的控件并重新测量。 */
void view_lvgl_perf_refresh(lv_obj_t *page, void *params);

#endif
