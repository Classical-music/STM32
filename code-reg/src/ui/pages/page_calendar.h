#ifndef __PAGE_CALENDAR_H__
#define __PAGE_CALENDAR_H__

#include "lvgl.h"

/* 创建日历页面：今日摘要 + 月历网格 + 月份切换，page 为路由器提供的页面容器。
 * 点击本月日期格可直接设置 RTC 日期；点击摘要卡时间文字打开弹窗设置时分秒。
 * 只在页面首次创建时调用；页面被路由缓存复用时不会再执行。 */
void view_calendar_init(lv_obj_t *page, void *params);

/* 页面离开显示（转缓存/入历史栈）时暂停秒级时钟定时器，重新显示时恢复。 */
void view_calendar_hide(lv_obj_t *page);
void view_calendar_show(lv_obj_t *page);

/* 页面被路由缓存淘汰、真正销毁前清空本模块的控件引用和定时器。 */
void view_calendar_destroy(lv_obj_t *page);

#endif
