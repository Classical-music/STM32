#include "views.h"

void btn_event_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  lv_obj_t *label = lv_obj_get_child(btn, 0);

  static int i = 0;
  lv_label_set_text_fmt(label, "Clicked %d times", ++i);
}

void views_init(void) {
  // 在这里初始化你的视图
  // 例如创建按钮、标签等控件

  // /* 创建测试控件：一个带标签的按钮 */
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 100, 100);
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Click");
  lv_obj_center(btn_label);

  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
}
