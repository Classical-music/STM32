#include "page_lvgl_perf.h"
#include "lvgl.h"
#include <stdio.h>

static void lvgl_perf_print(const char *name, uint32_t start, uint32_t end) {
  printf("[lvgl_perf] %-24s %lu ms\r\n", name,
         (unsigned long)(end - start));
}

#define LVGL_PERF_CREATE(parent, name, create_expr)                            \
  do {                                                                          \
    uint32_t start = lv_tick_get();                                            \
    lv_obj_t *obj = (create_expr);                                             \
    uint32_t end = lv_tick_get();                                              \
    if (obj != NULL)                                                           \
      lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);                                \
    lvgl_perf_print((name), start, end);                                       \
  } while (0)

/* 在页面容器下创建全部待测控件并打印各自的创建耗时。 */
static void lvgl_perf_build(lv_obj_t *page) {
  lv_obj_t *parent;
  uint32_t total_start;
  uint32_t total_end;

  /* 页面容器的尺寸、位置和基础样式已由路由器统一处理。 */
  parent = lv_obj_create(page);
  lv_obj_set_size(parent, 1, 1);
  lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);

  printf("[lvgl_perf] begin widget creation timing\r\n");
  total_start = lv_tick_get();

#if LV_USE_ARC
  LVGL_PERF_CREATE(parent, "lv_arc", lv_arc_create(parent));
#endif
#if LV_USE_BAR
  LVGL_PERF_CREATE(parent, "lv_bar", lv_bar_create(parent));
#endif
#if LV_USE_BTN
  LVGL_PERF_CREATE(parent, "lv_btn", lv_btn_create(parent));
#endif
#if LV_USE_BTNMATRIX
  LVGL_PERF_CREATE(parent, "lv_btnmatrix", lv_btnmatrix_create(parent));
#endif
#if LV_USE_CANVAS
  LVGL_PERF_CREATE(parent, "lv_canvas", lv_canvas_create(parent));
#endif
#if LV_USE_CHECKBOX
  LVGL_PERF_CREATE(parent, "lv_checkbox", lv_checkbox_create(parent));
#endif
#if LV_USE_DROPDOWN
  LVGL_PERF_CREATE(parent, "lv_dropdown", lv_dropdown_create(parent));
#endif
#if LV_USE_IMG
  LVGL_PERF_CREATE(parent, "lv_img", lv_img_create(parent));
#endif
#if LV_USE_LABEL
  LVGL_PERF_CREATE(parent, "lv_label", lv_label_create(parent));
#endif
#if LV_USE_LINE
  LVGL_PERF_CREATE(parent, "lv_line", lv_line_create(parent));
#endif
#if LV_USE_ROLLER
  LVGL_PERF_CREATE(parent, "lv_roller", lv_roller_create(parent));
#endif
#if LV_USE_SLIDER
  LVGL_PERF_CREATE(parent, "lv_slider", lv_slider_create(parent));
#endif
#if LV_USE_SWITCH
  LVGL_PERF_CREATE(parent, "lv_switch", lv_switch_create(parent));
#endif
#if LV_USE_TEXTAREA
  LVGL_PERF_CREATE(parent, "lv_textarea", lv_textarea_create(parent));
#endif
#if LV_USE_TABLE
  LVGL_PERF_CREATE(parent, "lv_table", lv_table_create(parent));
#endif
#if LV_USE_ANIMIMG
  LVGL_PERF_CREATE(parent, "lv_animimg", lv_animimg_create(parent));
#endif
#if LV_USE_CALENDAR
  LVGL_PERF_CREATE(parent, "lv_calendar", lv_calendar_create(parent));
#endif
#if LV_USE_CHART
  LVGL_PERF_CREATE(parent, "lv_chart", lv_chart_create(parent));
#endif
#if LV_USE_COLORWHEEL
  LVGL_PERF_CREATE(parent, "lv_colorwheel", lv_colorwheel_create(parent, false));
#endif
#if LV_USE_IMGBTN
  LVGL_PERF_CREATE(parent, "lv_imgbtn", lv_imgbtn_create(parent));
#endif
#if LV_USE_KEYBOARD
  LVGL_PERF_CREATE(parent, "lv_keyboard", lv_keyboard_create(parent));
#endif
#if LV_USE_LED
  LVGL_PERF_CREATE(parent, "lv_led", lv_led_create(parent));
#endif
#if LV_USE_LIST
  LVGL_PERF_CREATE(parent, "lv_list", lv_list_create(parent));
#endif
#if LV_USE_MENU
  LVGL_PERF_CREATE(parent, "lv_menu", lv_menu_create(parent));
#endif
#if LV_USE_METER
  LVGL_PERF_CREATE(parent, "lv_meter", lv_meter_create(parent));
#endif
#if LV_USE_MSGBOX
  {
    static const char *buttons[] = {"OK", ""};
    LVGL_PERF_CREATE(parent, "lv_msgbox",
                     lv_msgbox_create(parent, "Test", "Test", buttons, false));
  }
#endif
#if LV_USE_SPAN
  LVGL_PERF_CREATE(parent, "lv_spangroup", lv_spangroup_create(parent));
#endif
#if LV_USE_SPINBOX
  LVGL_PERF_CREATE(parent, "lv_spinbox", lv_spinbox_create(parent));
#endif
#if LV_USE_SPINNER
  LVGL_PERF_CREATE(parent, "lv_spinner", lv_spinner_create(parent, 1000, 60));
#endif
#if LV_USE_TABVIEW
  LVGL_PERF_CREATE(parent, "lv_tabview", lv_tabview_create(parent, LV_DIR_TOP, 40));
#endif
#if LV_USE_TILEVIEW
  LVGL_PERF_CREATE(parent, "lv_tileview", lv_tileview_create(parent));
#endif
#if LV_USE_WIN
  LVGL_PERF_CREATE(parent, "lv_win", lv_win_create(parent, 40));
#endif

  total_end = lv_tick_get();
  printf("[lvgl_perf] total widget creation %lu ms\r\n",
         (unsigned long)(total_end - total_start));
  printf("[lvgl_perf] end widget creation timing\r\n");
}

void view_lvgl_perf_init(lv_obj_t *page, void *params) {
  (void)params;
  lvgl_perf_build(page);
}

void view_lvgl_perf_refresh(lv_obj_t *page, void *params) {
  (void)params;
  /*
   * 本页的用途就是测量控件创建耗时，页面被路由缓存复用时必须清掉上一轮的
   * 控件重新创建，否则第二次进入不会有任何测量输出。
   */
  lv_obj_clean(page);
  lvgl_perf_build(page);
}

#undef LVGL_PERF_CREATE
