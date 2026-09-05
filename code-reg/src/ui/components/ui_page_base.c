#include "ui_page_base.h"

#define UI_PAGE_BASE_SYSTEM_BAR_HEIGHT 28
#define UI_PAGE_BASE_DOCK_HEIGHT 44
#define UI_PAGE_BASE_SIDE_PADDING 24
#define UI_PAGE_BASE_CONTENT_GAP 24

void ui_page_base_init(lv_obj_t *page) {
  lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
  lv_obj_set_layout(page, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_left(page, UI_PAGE_BASE_SIDE_PADDING, 0);
  lv_obj_set_style_pad_right(page, UI_PAGE_BASE_SIDE_PADDING, 0);
  lv_obj_set_style_pad_top(page, UI_PAGE_BASE_SYSTEM_BAR_HEIGHT, 0);
  lv_obj_set_style_pad_bottom(page, UI_PAGE_BASE_DOCK_HEIGHT, 0);
  lv_obj_set_style_pad_row(page, UI_PAGE_BASE_CONTENT_GAP, 0);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
}
