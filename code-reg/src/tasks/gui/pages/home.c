#include "home.h"
#include "beep/task_beep.h"
#include "lvgl.h"
#include <stdio.h>

// 清除提示的定时器回调
static void _clear_msg_cb(lv_timer_t *t) {
  lv_obj_t *lbl = (lv_obj_t *)t->user_data;
  if (lbl)
    lv_obj_del(lbl);
  lv_timer_del(t);
}

// 点击应用按钮时显示提示
static void app_btn_event_cb(lv_event_t *e) {
  const char *name = (const char *)lv_event_get_user_data(e);
  if (!name)
    name = "App";

  char buf[64];
  snprintf(buf, sizeof(buf), "Open: %s", name);

  lv_obj_t *msg = lv_label_create(lv_scr_act());
  lv_label_set_text(msg, buf);
  lv_obj_align(msg, LV_ALIGN_CENTER, 0, -20);
  lv_timer_t *t = lv_timer_create(_clear_msg_cb, 1500, msg);
  (void)t;

  // 播放短促按键音
  playBeep(20);
}

// 简洁手机主页：状态栏、图标网格、底部 Dock
void view_home_init(void *params) {
  lv_obj_t *scr = lv_scr_act();

  /* 本页不再创建本地状态栏，使用全局 system_bar */

  /* 应用图标网格 */
  const char *apps[] = {"Phone",    "Msg",    "Browser",
                        "Settings", "Camera", "Music"};
  const int app_count = sizeof(apps) / sizeof(apps[0]);

  int scr_w = lv_disp_get_hor_res(NULL);
  const int cols = 3;
  const int icon_w = 64;
  const int icon_h = 64;
  const int label_h = 18;
  const int margin_x = 12;
  const int spacing_x = (scr_w - margin_x * 2 - cols * icon_w) / (cols - 1);

  int start_y = 12; // 顶部留白

  for (int i = 0; i < app_count; i++) {
    int row = i / cols;
    int col = i % cols;
    int x = margin_x + col * (icon_w + spacing_x);
    int y = start_y + row * (icon_h + label_h + 12);

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, icon_w, icon_h + label_h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(btn);
    char icon_txt[4] = {0};
    icon_txt[0] = apps[i][0];
    lv_label_set_text(icon, icon_txt);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, apps[i]);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -6);

    lv_obj_add_event_cb(btn, app_btn_event_cb, LV_EVENT_CLICKED,
                        (void *)apps[i]);
  }

  /* 底部 Dock */
  lv_obj_t *dock = lv_obj_create(scr);
  lv_obj_set_size(dock, scr_w, 44);
  lv_obj_align(dock, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_clear_flag(dock, LV_OBJ_FLAG_SCROLLABLE);

  const char *dock_apps[] = {"Dial", "Contacts", "Msgs"};
  const uint32_t dock_colors[] = {0x3DDC84, 0x2196F3, 0xFF9800};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = lv_btn_create(dock);
    lv_obj_set_size(b, 56, 56);
    lv_obj_align(b, LV_ALIGN_LEFT_MID, 12 + i * 80, 0);

    // Android 风格：圆形、彩色背景、白色图标文字
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(dock_colors[i]), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(b, 0, 0);

    lv_obj_t *lbl = lv_label_create(b);
    lv_label_set_text(lbl, dock_apps[i]);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

    lv_obj_add_event_cb(b, app_btn_event_cb, LV_EVENT_CLICKED,
                        (void *)dock_apps[i]);
  }
}
