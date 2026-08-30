#include "home.h"
#include "../common/router.h"
#include "../components/ui_button.h"
#include "beep/task_beep.h"
#include "lvgl.h"

/*
 * 首页菜单项配置。
 * name 用于显示菜单名称，route 用于指定点击后进入的页面路由。
 * route 为空字符串时，按钮只响应点击，不进行页面跳转。
 */
typedef struct {
  const char *name;
  const char *route;
} home_menu_item_t;

/*
 * 首页菜单列表。
 * 新增菜单时只需在此处增加配置，无需修改按钮创建和事件处理逻辑。
 * 路由名称必须与 views_init() 中注册的路由名称保持一致。
 */
static const home_menu_item_t home_menu[] = {
    {"LED/Beep", "led_beep"},
    {"Msg", ""},
    {"Browser", ""},
    {"Settings", ""},
    {"Camera", ""},
    {"Music", ""},
};

/*
 * 所有首页菜单按钮共用的点击事件。
 * 事件的 user_data 指向对应的 home_menu_item_t 静态菜单项。
 */
static void menu_btn_event_cb(lv_event_t *e) {
  const home_menu_item_t *item =
      (const home_menu_item_t *)lv_event_get_user_data(e);

  /* 非空路由使用 push 导航，以便通过底部 Back 按钮返回首页。 */
  if (item && item->route && item->route[0] != '\0')
    router_push(item->route, NULL);

  /* 无论是否发生页面跳转，点击按钮时均播放按键反馈音。 */
  playBeep(20);
}

/*
 * 创建首页应用菜单。
 * 页面 screen 由路由器创建并加载，本函数只负责在当前 screen 上创建控件。
 * 顶部系统栏和底部 Dock 均为全局组件，不在首页中重复创建。
 */
void view_home_init(void *params) {
  lv_obj_t *scr = lv_scr_act();

  /* 首页当前不使用路由参数。 */
  (void)params;

  /*
   * 手动实现类似 Flex 的横向换行布局：
   * 根据容器宽度计算一行能容纳的按钮数量，并将剩余空间均匀分配为列间距。
   * 这样不依赖 LVGL 的 Flex 模块，屏幕宽度变化时仍能自动重新计算布局。
   */
  const int menu_count = sizeof(home_menu) / sizeof(home_menu[0]);
  const int button_w = 64;
  const int button_h = 82;
  const int margin_x = 12;
  const int start_y = 12;
  const int row_gap = 12;
  const int min_column_gap = 8;
  const int content_w = lv_obj_get_width(scr) - margin_x * 2;
  int columns = (content_w + min_column_gap) / (button_w + min_column_gap);

  if (columns < 1)
    columns = 1;
  if (columns > menu_count)
    columns = menu_count;

  const int column_gap = columns > 1
                             ? (content_w - columns * button_w) / (columns - 1)
                             : 0;

  for (int i = 0; i < menu_count; i++) {
    const home_menu_item_t *item = &home_menu[i];
    const int row = i / columns;
    const int column = i % columns;
    const int x = margin_x + column * (button_w + column_gap);
    const int y = start_y + row * (button_h + row_gap);
    lv_obj_t *btn = ui_button_create(scr, item->name);

    lv_obj_set_size(btn, button_w, button_h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    /* 将菜单项地址传给公共点击回调，用于读取对应页面路由。 */
    lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)item);
  }
}
