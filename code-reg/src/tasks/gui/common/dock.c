#include "dock.h"
#include "router.h"
#include "beep/task_beep.h"
#include "lvgl.h"

/*
 * 全局 Dock 容器。
 * Dock 创建在 LVGL 顶层图层 lv_layer_top() 上，不属于任何业务页面，
 * 因此 router 切换或销毁活动 screen 时，Dock 会继续显示。
 */
static lv_obj_t *g_dock = NULL;

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

/* 右侧按钮：暂时仅保留按键蜂鸣。 */
static void recent_btn_event_cb(lv_event_t *event) {
  (void)event;
  playBeep(20);
}

/*
 * 初始化跨页面共用的底部 Dock。
 * 此函数设计为幂等调用：首次调用创建组件，后续调用直接返回，避免重复创建。
 */
void dock_init(void) {
  /* 三个系统导航按钮：后退、首页、最近任务（暂未实现）。 */
  static const char *dock_apps[] = {"Back", "Home", "Recent"};
  static lv_event_cb_t dock_events[] = {back_btn_event_cb, home_btn_event_cb,
                                        recent_btn_event_cb};

  /* 每个按钮对应的背景颜色：绿色、蓝色、橙色。 */
  static const uint32_t dock_colors[] = {0x3DDC84, 0x2196F3, 0xFF9800};
  lv_obj_t *layer;
  int i;

  /* 已创建时不再重复初始化。 */
  if (g_dock)
    return;

  /*
   * 挂载到 LVGL 顶层，而不是 lv_scr_act()。
   * 这样 Dock 不会随业务页面 screen 的删除而销毁。
   */
  layer = lv_layer_top();
  g_dock = lv_obj_create(layer);
  lv_obj_set_size(g_dock, lv_disp_get_hor_res(NULL), 44);
  lv_obj_align(g_dock, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_clear_flag(g_dock, LV_OBJ_FLAG_SCROLLABLE);

  /* 根据配置数组依次创建三个固定入口按钮。 */
  for (i = 0; i < 3; i++) {
    lv_obj_t *button = lv_btn_create(g_dock);
    lv_obj_t *label;

    /* 设置圆形按钮的尺寸、位置及背景样式。 */
    lv_obj_set_size(button, 56, 56);
    lv_obj_align(button, LV_ALIGN_LEFT_MID, 12 + i * 80, 0);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(dock_colors[i]), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 0, 0);

    /* 在按钮中心显示应用名称。 */
    label = lv_label_create(button);
    lv_label_set_text(label, dock_apps[i]);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);

    /* 每个按钮绑定各自的系统导航行为。 */
    lv_obj_add_event_cb(button, dock_events[i], LV_EVENT_CLICKED, NULL);
  }
}
