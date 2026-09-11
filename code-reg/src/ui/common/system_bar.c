#include "system_bar.h"
#include "lvgl.h"
#include <stdio.h>
// 设备可能不支持标准 time()/localtime(), 使用内部计时器从 12:00 启动

static lv_obj_t *g_bar = NULL;
static lv_obj_t *g_time_label = NULL;
static lv_obj_t *g_bat_label = NULL;
static lv_timer_t *g_timer = NULL;

static int g_hour = 12;
static int g_min = 0;
static int g_sec = 0;

static void _update_time_label(void) {
  char buf[6] = "00:00";
  snprintf(buf, sizeof(buf), "%02d:%02d", g_hour, g_min);
  if (g_time_label)
    lv_label_set_text(g_time_label, buf);
}

static void _timer_cb(lv_timer_t *t) {
  (void)t;
  if (!g_time_label)
    return;

  // 内部计时：每秒增加，满 60 秒进 1 分，满 60 分进 1 时
  g_sec++;
  if (g_sec >= 60) {
    g_sec = 0;
    g_min++;
    if (g_min >= 60) {
      g_min = 0;
      g_hour = (g_hour + 1) % 24;
    }
    _update_time_label();
  }
}

void system_bar_init(void) {
  if (g_bar)
    return; // 已初始化

  /* 作为默认屏幕的子节点创建，与页面处于同一层级，不使用顶层图层。 */
  g_bar = lv_obj_create(lv_scr_act());

  /* 删除主题默认样式，避免圆角、边框和内边距在四周漏出底层背景。 */
  lv_obj_remove_style_all(g_bar);

  /* 系统栏占满屏幕宽度，固定高度，紧贴屏幕最顶部。 */
  lv_obj_set_size(g_bar, lv_disp_get_hor_res(NULL), SYSTEM_BAR_HEIGHT);
  lv_obj_set_pos(g_bar, 0, 0);
  lv_obj_clear_flag(g_bar, LV_OBJ_FLAG_SCROLLABLE);

  /* 不透明深色背景，文字颜色由子控件继承。 */
  lv_obj_set_style_bg_color(g_bar, lv_color_hex(0x323232), 0);
  lv_obj_set_style_bg_opa(g_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(g_bar, lv_color_hex(0xFFFFFF), 0);

  g_time_label = lv_label_create(g_bar);
  lv_label_set_text(g_time_label, "00:00");
  lv_obj_align(g_time_label, LV_ALIGN_LEFT_MID, 8, 0);

  g_bat_label = lv_label_create(g_bar);
  lv_label_set_text(g_bat_label, "100%");
  lv_obj_align(g_bat_label, LV_ALIGN_RIGHT_MID, -8, 0);

  // 每秒递增内部时钟并更新显示
  g_timer = lv_timer_create(_timer_cb, 1000, NULL);

  // 立即刷新一次
  _update_time_label();
}

// 不再提供外部设置时间的接口，系统栏始终使用系统时钟

void system_bar_show(void) {
  if (g_bar)
    lv_obj_clear_flag(g_bar, LV_OBJ_FLAG_HIDDEN);
}

void system_bar_hide(void) {
  if (g_bar)
    lv_obj_add_flag(g_bar, LV_OBJ_FLAG_HIDDEN);
}
