#ifndef __PAGE_STATUS_H__
#define __PAGE_STATUS_H__

#include "lvgl.h"

/* 创建设备状态页面（芯片温度 + 参考电压），page 为路由器提供的页面容器。
 * 只在页面首次创建时调用；页面被路由缓存复用时不会再执行。 */
void view_status_init(lv_obj_t *page, void *params);

/* 页面离开显示（转缓存/入历史栈）时暂停 1s 刷新定时器，重新显示时恢复。 */
void view_status_hide(lv_obj_t *page);
void view_status_show(lv_obj_t *page);

/* 页面被路由缓存淘汰、真正销毁前删除定时器并清空本模块的控件引用。 */
void view_status_destroy(lv_obj_t *page);

#endif
