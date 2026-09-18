#include "system_bar.h"
#include "rtc/task_rtc.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *g_bar = NULL;
static lv_obj_t *g_title_label = NULL;
static lv_obj_t *g_time_label = NULL;
static lv_timer_t *g_timer = NULL;

/* 最近一次设置的标题，初始化前后都保存在这里，避免状态丢失。 */
static char g_title[SYSTEM_BAR_TITLE_MAX_LEN] = "";

static void _update_time_label(void) {
  char buf[6] = "00:00";
  rtc_time_t t;
  rtcGetLocalTime(&t);
  snprintf(buf, sizeof(buf), "%02u:%02u", t.hour, t.minute);
  if (g_time_label)
    lv_label_set_text(g_time_label, buf);
}

static void _timer_cb(lv_timer_t *t) {
  (void)t;
  if (!g_time_label)
    return;

  // 每秒从 RTC 服务层读取当前本地时间并刷新显示
  _update_time_label();
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

  /* 左侧：页面标题（使用全局默认字体 montserrat_18）。 */
  g_title_label = lv_label_create(g_bar);
  lv_label_set_text(g_title_label, g_title);
  lv_obj_align(g_title_label, LV_ALIGN_LEFT_MID, 12, 0);

  /* 右侧：时间。 */
  g_time_label = lv_label_create(g_bar);
  lv_label_set_text(g_time_label, "00:00");
  lv_obj_align(g_time_label, LV_ALIGN_RIGHT_MID, -12, 0);

  // 每秒递增内部时钟并更新显示
  g_timer = lv_timer_create(_timer_cb, 1000, NULL);

  // 立即刷新一次
  _update_time_label();
}

void system_bar_set_title(const char *title) {
  if (title)
    strncpy(g_title, title, sizeof(g_title) - 1);
  else
    g_title[0] = '\0';
  g_title[sizeof(g_title) - 1] = '\0';

  if (g_title_label)
    lv_label_set_text(g_title_label, g_title);
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
