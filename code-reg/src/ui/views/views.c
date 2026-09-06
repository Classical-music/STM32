#include "views.h"
#include "../common/router.h"
#include "../common/system_bar.h"
#include "../common/dock.h"
#include "../common/ui_virtual_keyboard.h"
#include "../pages/page_home.h"
#include "../pages/page_led_beep.h"
#include "../pages/page_lvgl_perf.h"

void views_init(void) {
  static int initialized;

  if (initialized)
    return;
  initialized = 1;

  // 初始化路由子系统
  router_init();

  // 初始化全局系统栏（确保它在所有页面之上）
  system_bar_init();
  dock_init();

  // 注册页面
  router_register("home", view_home_init);
  router_register("led_beep", view_led_beep_init);
  router_register("lvgl_perf", view_lvgl_perf_init);

  // 导航到首页
  router_navigate("home", NULL);

  // 首页创建完成后，异步预创建全局虚拟键盘。
  ui_virtual_keyboard_init_async();
}
