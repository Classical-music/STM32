#include "views.h"
#include "../common/dock.h"
#include "../common/router.h"
#include "../common/system_bar.h"
#include "../pages/page_beep.h"
#include "../pages/page_calendar.h"
#include "../pages/page_home.h"
#include "../pages/page_led.h"
#include "../pages/page_lvgl_perf.h"
#include "../pages/page_status.h"

/* 默认屏幕的背景色，与页面背景保持一致，避免首帧出现主题默认的白底。 */
#define VIEWS_ROOT_BG_COLOR 0xF2F2F2

void views_init(void) {
  static int initialized;
  lv_obj_t *screen;

  if (initialized)
    return;
  initialized = 1;

  /*
   * 系统栏、Dock 和页面都挂在 LVGL 默认屏幕（lv_scr_act()）下，各模块内部
   * 自行取得该屏幕，views 不再向下传递根节点。这里只一次性设置屏幕背景，
   * 避免首页异步创建完成前露出主题默认的白底。
   */
  screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(VIEWS_ROOT_BG_COLOR), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // 先创建常驻的系统栏和 Dock，它们不随页面切换而重建
  system_bar_init();
  dock_init();

  // 初始化路由子系统，页面容器由路由器按需创建在默认屏幕上
  router_init();

  // 注册页面（第二个参数为系统栏标题，hide/show 用于离开/进入显示时暂停、
  // 恢复页面自身的定时器，没有定时器的页面传 NULL 即可）
  router_register_ex("home", "Home", view_home_init, NULL, NULL, NULL, NULL);
  router_register_ex("led", "LED Control", view_led_init, NULL, NULL, NULL,
                     view_led_destroy);
  // beep 有一次性播放完成定时器：隐藏时暂停、显示时恢复；
  // 模块内保存了控件引用，被缓存淘汰销毁时需要清空，
  // 否则下次重建会误判控件已存在。
  router_register_ex("beep", "Buzzer Control", view_beep_init, NULL,
                     view_beep_hide, view_beep_show, view_beep_destroy);
  // calendar 有 1s 时钟定时器：隐藏时暂停、显示时恢复，
  // 恢复后 LVGL 自动补发到期的回调，缓存复用时无需额外 refresh；
  // 模块保存了控件引用和定时器，销毁时需要清空。
  router_register_ex("calendar", "Calendar", view_calendar_init, NULL,
                     view_calendar_hide, view_calendar_show,
                     view_calendar_destroy);
  // lvgl_perf 的用途是测量控件创建耗时，缓存复用时需要重建控件重新测量。
  router_register_ex("lvgl_perf", "LVGL Perf", view_lvgl_perf_init,
                     view_lvgl_perf_refresh, NULL, NULL, NULL);
  // status 有 1s 刷新定时器：隐藏时暂停、显示时恢复，
  // 恢复后 LVGL 自动补发到期的回调，缓存复用时无需额外 refresh；
  // 模块保存了控件引用和定时器，销毁时需要清空。
  router_register_ex("status", "Device Status", view_status_init, NULL,
                     view_status_hide, view_status_show, view_status_destroy);

  // 导航到首页
  router_navigate("home", NULL);
}
