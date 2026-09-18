#include "page_home.h"
#include "../common/router.h"
#include "beep/task_beep.h"
#include "lvgl.h"

/*
 * 首页菜单项配置。
 * name 用于显示菜单名称，route 用于指定点击后进入的页面路由，
 * x/y 是卡片相对页面内容区左上角的固定位置。
 * route 为空字符串时，卡片只响应点击蜂鸣，不进行页面跳转（页面尚未实现）。
 */
typedef struct {
  const char *name;
  const char *route;
  lv_coord_t x;
  lv_coord_t y;
} home_menu_item_t;

/*
 * 页面内容区为 432 x 636（页面 480 x 684 减去四周 24 内边距），
 * 卡片按两列四行固定摆放：列 x 为 0 / 224，行 y 为 0 / 162 / 324 / 486。
 */
static const home_menu_item_t home_menu_items[] = {
    {"LED Ctrl", "led", 0, 0},
    {"Buzzer", "beep", 224, 0},
    {"Calendar", "calendar", 0, 162},
    {"RTC Clock", "", 224, 162},
    {"File Mgr", "", 0, 324},
    {"Media", "", 224, 324},
    {"LVGL Perf", "lvgl_perf", 0, 486},
};

/* 卡片固定尺寸。 */
#define HOME_CARD_WIDTH 208
#define HOME_CARD_HEIGHT 150

/* 卡片外观：白色圆角加柔和投影，与浅灰页面背景区分。 */
#define HOME_CARD_RADIUS 12
#define HOME_CARD_BG_COLOR 0xFFFFFF
#define HOME_CARD_PRESSED_BG_COLOR 0xE6E6E6
#define HOME_CARD_SHADOW_COLOR 0x9AA0A6
#define HOME_CARD_SHADOW_WIDTH 16
#define HOME_CARD_SHADOW_OPA LV_OPA_20
#define HOME_CARD_SHADOW_OFS_Y 4

/* 所有卡片共用的样式，只在首次创建首页时构建一次。 */
static lv_style_t g_home_card_style;
static bool g_home_card_style_initialized;

static lv_style_t *home_card_style_get(void) {
  if (g_home_card_style_initialized)
    return &g_home_card_style;

  lv_style_init(&g_home_card_style);

  lv_style_set_radius(&g_home_card_style, HOME_CARD_RADIUS);
  lv_style_set_bg_color(&g_home_card_style, lv_color_hex(HOME_CARD_BG_COLOR));
  lv_style_set_bg_opa(&g_home_card_style, LV_OPA_COVER);
  lv_style_set_border_width(&g_home_card_style, 0);
  lv_style_set_shadow_width(&g_home_card_style, HOME_CARD_SHADOW_WIDTH);
  lv_style_set_shadow_color(&g_home_card_style,
                            lv_color_hex(HOME_CARD_SHADOW_COLOR));
  lv_style_set_shadow_opa(&g_home_card_style, HOME_CARD_SHADOW_OPA);
  lv_style_set_shadow_ofs_y(&g_home_card_style, HOME_CARD_SHADOW_OFS_Y);
  lv_style_set_pad_all(&g_home_card_style, 0);
  lv_style_set_text_color(&g_home_card_style, lv_color_black());

  g_home_card_style_initialized = true;
  return &g_home_card_style;
}

/* 菜单卡片点击回调。 */
static void home_menu_btn_event_cb(lv_event_t *event) {
  const home_menu_item_t *item =
      (const home_menu_item_t *)lv_event_get_user_data(event);

  if (item != NULL && item->route != NULL && item->route[0] != '\0')
    router_push(item->route, NULL);

  playBeep(20);
}

/*
 * 创建首页菜单：两列白色圆角卡片网格。
 * page 由路由器创建并已定位在系统栏和 Dock 之间，卡片尺寸和位置都在
 * home_menu_items 中写死，不依赖运行时读回页面尺寸。
 */
void view_home_init(lv_obj_t *page, void *params) {
  uint32_t item_count;
  uint32_t i;

  (void)params;

  item_count = sizeof(home_menu_items) / sizeof(home_menu_items[0]);

  for (i = 0; i < item_count; i++) {
    lv_obj_t *card = lv_btn_create(page);
    lv_obj_t *label;

    /* 丢弃主题按钮样式，卡片外观完全由共用样式决定。 */
    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, home_card_style_get(), 0);
    lv_obj_set_size(card, HOME_CARD_WIDTH, HOME_CARD_HEIGHT);
    lv_obj_set_pos(card, home_menu_items[i].x, home_menu_items[i].y);

    /* 主题样式清空后按下反馈丢失，用按下态背景色补回。 */
    lv_obj_set_style_bg_color(card, lv_color_hex(HOME_CARD_PRESSED_BG_COLOR),
                              LV_STATE_PRESSED);

    label = lv_label_create(card);
    lv_label_set_text_static(label, home_menu_items[i].name);
    lv_obj_center(label);

    lv_obj_add_event_cb(card, home_menu_btn_event_cb, LV_EVENT_CLICKED,
                        (void *)&home_menu_items[i]);
  }
}
