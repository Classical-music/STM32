#include "page_home.h"
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

static const home_menu_item_t home_menu_items[] = {
    {"LED / Beep", "led_beep"},
    {"LVGL Perf", "lvgl_perf"},
};

/* 菜单按钮点击回调。 */
static void home_menu_btn_event_cb(lv_event_t *event) {
  const home_menu_item_t *item =
      (const home_menu_item_t *)lv_event_get_user_data(event);

  if (item != NULL && item->route != NULL && item->route[0] != '\0')
    router_push(item->route, NULL);

  playBeep(20);
}

/*
 * 创建首页菜单。
 * 使用 Flex 横向排列按钮，容器宽度不足时自动换行。
 */
void view_home_init(void *params) {
  lv_obj_t *scr = lv_scr_act();

  (void)params;

  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_left(scr, 12, 0);
  lv_obj_set_style_pad_right(scr, 12, 0);
  lv_obj_set_style_pad_top(scr, 12, 0);
  lv_obj_set_style_pad_row(scr, 12, 0);
  lv_obj_set_style_pad_column(scr, 8, 0);

  for (uint32_t i = 0; i < sizeof(home_menu_items) / sizeof(home_menu_items[0]);
       i++) {
    lv_obj_t *button = ui_button_create(scr, home_menu_items[i].name);

    lv_obj_set_size(button, 64, 82);
    lv_obj_add_event_cb(button, home_menu_btn_event_cb, LV_EVENT_CLICKED,
                        (void *)&home_menu_items[i]);
  }
}
