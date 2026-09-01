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
   * 使用 LVGL Flex 布局管理首页按钮：
   * - 按行排列按钮，空间不足时自动换行；
   * - 子控件按照创建顺序从左到右、从上到下排列；
   * - 所有按钮从容器左上角开始布局。
   */
  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  /* 设置 Flex 容器的内边距和按钮之间的间距。 */
  lv_obj_set_style_pad_left(scr, 12, 0);
  lv_obj_set_style_pad_right(scr, 12, 0);
  lv_obj_set_style_pad_top(scr, 12, 0);
  lv_obj_set_style_pad_row(scr, 12, 0);
  lv_obj_set_style_pad_column(scr, 8, 0);

  const int menu_count = sizeof(home_menu) / sizeof(home_menu[0]);
  const int button_w = 64;
  const int button_h = 82;

  for (int i = 0; i < menu_count; i++) {
    const home_menu_item_t *item = &home_menu[i];
    lv_obj_t *btn = ui_button_create(scr, item->name);

    lv_obj_set_size(btn, button_w, button_h);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)item);
  }
}
