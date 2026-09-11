#include "dock.h"
#include "router.h"
#include "beep/task_beep.h"
#include "lvgl.h"

/*
 * 全局 Dock 容器。
 * Dock 创建在默认屏幕（lv_scr_act()）上，和业务页面是同级子节点，不属于任何
 * 页面，因此 router 切换页面时不会重建，也不会随页面一起被销毁。
 */
static lv_obj_t *g_dock = NULL;

/* 导航按钮的数量、尺寸和间距，用于一次算出居中排列的固定坐标。 */
#define DOCK_BUTTON_COUNT 3
#define DOCK_BUTTON_WIDTH 100
#define DOCK_BUTTON_HEIGHT 36
#define DOCK_BUTTON_GAP 20

/* Dock 栏与导航按钮的背景色：按钮比栏体略亮，用于区分可点击区域。 */
#define DOCK_BG_COLOR 0x383838
#define DOCK_BUTTON_BG_COLOR 0x4A4A4A

/* 按钮共用样式：只在首次初始化时构建，三个按钮重复使用。 */
static lv_style_t g_dock_button_style;
static bool g_dock_styles_initialized;

static void dock_init_styles(void) {
  if (g_dock_styles_initialized)
    return;

  lv_style_init(&g_dock_button_style);

  /* 胶囊形按钮：统一深灰填充、无边框、无阴影、无内边距、白色文字。 */
  lv_style_set_radius(&g_dock_button_style, LV_RADIUS_CIRCLE);
  lv_style_set_bg_color(&g_dock_button_style, lv_color_hex(DOCK_BUTTON_BG_COLOR));
  lv_style_set_bg_opa(&g_dock_button_style, LV_OPA_COVER);
  lv_style_set_border_width(&g_dock_button_style, 0);
  lv_style_set_shadow_width(&g_dock_button_style, 0);
  lv_style_set_pad_all(&g_dock_button_style, 0);
  lv_style_set_text_color(&g_dock_button_style, lv_color_hex(0xFFFFFF));

  g_dock_styles_initialized = true;
}

/* 左侧按钮：根据路由历史返回上一页面；根页面没有历史时保持不变。 */
static void back_btn_event_cb(lv_event_t *event) {
  (void)event;
  router_pop();
  playBeep(20);
}

/* 中间按钮：返回首页并清空历史，避免 Home 后再按后退回到旧页面。 */
static void home_btn_event_cb(lv_event_t *event) {
  (void)event;
  router_reset("home", NULL);
  playBeep(20);
}

/* 右侧按钮：菜单入口，暂时仅保留按键蜂鸣。 */
static void menu_btn_event_cb(lv_event_t *event) {
  (void)event;
  playBeep(20);
}

/*
 * 初始化跨页面共用的底部 Dock。
 * 此函数设计为幂等调用：首次调用创建组件，后续调用直接返回，避免重复创建。
 */
void dock_init(void) {
  /* 三个系统导航按钮：返回、首页、菜单。 */
  static const char *dock_apps[DOCK_BUTTON_COUNT] = {"Back", "Home", "Menu"};
  static lv_event_cb_t dock_events[DOCK_BUTTON_COUNT] = {
      back_btn_event_cb, home_btn_event_cb, menu_btn_event_cb};

  lv_coord_t dock_width;
  lv_coord_t button_x;
  lv_coord_t button_y;
  int i;

  /* 已创建时不再重复初始化。 */
  if (g_dock)
    return;

  dock_init_styles();

  /* 作为默认屏幕的子节点创建，与系统栏、页面处于同一层级。 */
  g_dock = lv_obj_create(lv_scr_act());

  /* 删除主题默认样式，避免圆角、边框和内边距在四周漏出底层背景。 */
  lv_obj_remove_style_all(g_dock);

  /* Dock 占满屏幕宽度，固定高度，紧贴屏幕最底部。 */
  dock_width = lv_disp_get_hor_res(NULL);
  lv_obj_set_size(g_dock, dock_width, DOCK_HEIGHT);
  lv_obj_set_pos(g_dock, 0, lv_disp_get_ver_res(NULL) - DOCK_HEIGHT);
  lv_obj_clear_flag(g_dock, LV_OBJ_FLAG_SCROLLABLE);

  /* 不透明深色背景，白色文字由子控件继承。 */
  lv_obj_set_style_bg_color(g_dock, lv_color_hex(DOCK_BG_COLOR), 0);
  lv_obj_set_style_bg_opa(g_dock, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(g_dock, lv_color_hex(0xFFFFFF), 0);

  /*
   * 按钮尺寸固定，因此居中排列的起始坐标可以一次算出，
   * 直接用固定坐标定位，避免 Flex 布局在刷新阶段重新计算所有子对象。
   */
  button_x = (dock_width - (DOCK_BUTTON_COUNT * DOCK_BUTTON_WIDTH +
                            (DOCK_BUTTON_COUNT - 1) * DOCK_BUTTON_GAP)) /
             2;
  button_y = (DOCK_HEIGHT - DOCK_BUTTON_HEIGHT) / 2;

  /* 根据配置数组依次创建三个固定入口按钮。 */
  for (i = 0; i < DOCK_BUTTON_COUNT; i++) {
    lv_obj_t *button = lv_btn_create(g_dock);
    lv_obj_t *label;

    /* 共用样式覆盖主题外观，同时保留主题自带的按下反馈。 */
    lv_obj_add_style(button, &g_dock_button_style, 0);
    lv_obj_set_size(button, DOCK_BUTTON_WIDTH, DOCK_BUTTON_HEIGHT);
    lv_obj_set_pos(button,
                   button_x + i * (DOCK_BUTTON_WIDTH + DOCK_BUTTON_GAP),
                   button_y);

    /* 在按钮中心显示名称；文本来自静态数组，无需复制。 */
    label = lv_label_create(button);
    lv_label_set_text_static(label, dock_apps[i]);
    lv_obj_center(label);

    /* 每个按钮绑定各自的系统导航行为。 */
    lv_obj_add_event_cb(button, dock_events[i], LV_EVENT_CLICKED, NULL);
  }
}
