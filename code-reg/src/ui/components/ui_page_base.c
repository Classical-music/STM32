#include "ui_page_base.h"
#include "../common/dock.h"
#include "../common/system_bar.h"
#include <lvgl.h>

/* 页面内容四周统一内边距。 */
#define UI_PAGE_BASE_PADDING 24

/* 页面中相邻内容块的垂直间距。 */
#define UI_PAGE_BASE_CONTENT_GAP 24

/* 页面背景色：浅灰，与深色的系统栏、Dock 区分。 */
#define UI_PAGE_BASE_BG_COLOR 0xDADADA

/* 所有页面共用的样式，只在首次创建页面时构建一次。 */
static lv_style_t g_page_style;
static bool g_page_style_initialized;

static lv_style_t *page_style_get(void) {
  if (g_page_style_initialized)
    return &g_page_style;

  lv_style_init(&g_page_style);

  /* 不透明浅灰背景、统一内边距、黑色文字，其余样式一律不保留。 */
  lv_style_set_bg_color(&g_page_style, lv_color_hex(UI_PAGE_BASE_BG_COLOR));
  lv_style_set_bg_opa(&g_page_style, LV_OPA_COVER);
  lv_style_set_pad_all(&g_page_style, UI_PAGE_BASE_PADDING);
  lv_style_set_text_color(&g_page_style, lv_color_black());

  g_page_style_initialized = true;
  return &g_page_style;
}

lv_obj_t *ui_page_base_create(lv_obj_t *root) {
  lv_obj_t *page = lv_obj_create(root);
  if (page == NULL)
    return NULL;

  /* 丢弃主题的 card 默认样式，页面外观完全由共用样式决定。 */
  lv_obj_remove_style_all(page);
  lv_obj_add_style(page, page_style_get(), 0);

  /*
   * 页面是根 screen 的普通子节点，因此可以直接指定尺寸和位置：
   * 宽度占满屏幕，高度为屏幕高度减去系统栏和 Dock，顶部紧贴系统栏下方。
   * 与把页面自身当作 screen 的做法不同，这里不会与 LVGL 对 screen 尺寸的
   * 内部约定冲突。
   */
  lv_obj_set_size(page, lv_disp_get_hor_res(NULL),
                  lv_disp_get_ver_res(NULL) - SYSTEM_BAR_HEIGHT - DOCK_HEIGHT);
  lv_obj_set_pos(page, 0, SYSTEM_BAR_HEIGHT);

  /* 页面内容纵向依次排列，子控件由各页面直接创建在 page 上。 */
  lv_obj_set_layout(page, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(page, UI_PAGE_BASE_CONTENT_GAP, 0);

  /* 内容超高时直接裁剪，避免页面绘制到系统栏或 Dock 上。 */
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  return page;
}
