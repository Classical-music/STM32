#include "ui_popup.h"

/* 遮罩压暗页面的程度。 */
#define UI_POPUP_MASK_OPA LV_OPA_60

/* 主区域外观：白底圆角，与页面卡片一致。 */
#define UI_POPUP_BODY_COLOR 0xFFFFFF
#define UI_POPUP_BODY_RADIUS 16

/* 遮罩和主区域各用一套共用样式，只在首次创建弹窗时构建。 */
static lv_style_t g_popup_styles[2];
static bool g_popup_styles_initialized;

enum {
  UI_POPUP_STYLE_MASK = 0,
  UI_POPUP_STYLE_BODY,
};

static void ui_popup_styles_init(void) {
  if (g_popup_styles_initialized)
    return;

  lv_style_init(&g_popup_styles[UI_POPUP_STYLE_MASK]);
  lv_style_set_bg_color(&g_popup_styles[UI_POPUP_STYLE_MASK], lv_color_black());
  lv_style_set_bg_opa(&g_popup_styles[UI_POPUP_STYLE_MASK], UI_POPUP_MASK_OPA);

  lv_style_init(&g_popup_styles[UI_POPUP_STYLE_BODY]);
  lv_style_set_bg_color(&g_popup_styles[UI_POPUP_STYLE_BODY],
                        lv_color_hex(UI_POPUP_BODY_COLOR));
  lv_style_set_bg_opa(&g_popup_styles[UI_POPUP_STYLE_BODY], LV_OPA_COVER);
  lv_style_set_radius(&g_popup_styles[UI_POPUP_STYLE_BODY],
                      UI_POPUP_BODY_RADIUS);
  lv_style_set_border_width(&g_popup_styles[UI_POPUP_STYLE_BODY], 0);
  lv_style_set_text_color(&g_popup_styles[UI_POPUP_STYLE_BODY],
                          lv_color_black());

  g_popup_styles_initialized = true;
}

/* 点击遮罩空白处：允许点外关闭时收起弹窗。主区域自己吞掉了点击，不会走到这里。 */
static void ui_popup_mask_event_cb(lv_event_t *event) {
  lv_obj_t *mask = lv_event_get_current_target(event);

  if (lv_obj_has_flag(mask, LV_OBJ_FLAG_USER_1))
    lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t *ui_popup_create(lv_obj_t *parent, lv_coord_t body_width,
                          lv_coord_t body_height) {
  lv_obj_t *mask;
  lv_obj_t *body;

  if (parent == NULL)
    return NULL;

  ui_popup_styles_init();

  mask = lv_obj_create(parent);
  if (mask == NULL)
    return NULL;

  /* 遮罩铺满 parent 的内容区，不额外占位也不滚动。 */
  lv_obj_remove_style_all(mask);
  lv_obj_add_style(mask, &g_popup_styles[UI_POPUP_STYLE_MASK], 0);
  lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
  lv_obj_set_pos(mask, 0, 0);
  lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(mask, LV_OBJ_FLAG_CLICKABLE);
  /* USER_1 表示“点遮罩空白处关闭”，创建时默认开启。 */
  lv_obj_add_flag(mask, LV_OBJ_FLAG_USER_1);
  lv_obj_add_event_cb(mask, ui_popup_mask_event_cb, LV_EVENT_CLICKED, NULL);

  body = lv_obj_create(mask);
  if (body == NULL) {
    lv_obj_del(mask);
    return NULL;
  }

  lv_obj_remove_style_all(body);
  lv_obj_add_style(body, &g_popup_styles[UI_POPUP_STYLE_BODY], 0);
  lv_obj_set_size(body, body_width, body_height);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
  /* 主区域吃掉点击，避免点弹窗内部被误判成点空白关闭。 */
  lv_obj_add_flag(body, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(body);

  /* 遮罩的 user_data 存放主区域，组件不额外分配内存。 */
  lv_obj_set_user_data(mask, body);

  /* 创建后先隐藏，由使用方在合适时机 open。 */
  lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);

  return mask;
}

lv_obj_t *ui_popup_get_body(lv_obj_t *popup) {
  if (popup == NULL)
    return NULL;

  return (lv_obj_t *)lv_obj_get_user_data(popup);
}

void ui_popup_open(lv_obj_t *popup) {
  if (popup == NULL)
    return;

  lv_obj_clear_flag(popup, LV_OBJ_FLAG_HIDDEN);
  /* 页面里弹窗之后可能还创建了别的控件，显示时保证盖在最上层。 */
  lv_obj_move_foreground(popup);
}

void ui_popup_close(lv_obj_t *popup) {
  if (popup == NULL)
    return;

  lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN);
}

bool ui_popup_is_open(lv_obj_t *popup) {
  if (popup == NULL)
    return false;

  return !lv_obj_has_flag(popup, LV_OBJ_FLAG_HIDDEN);
}

void ui_popup_set_dismissible(lv_obj_t *popup, bool dismissible) {
  if (popup == NULL)
    return;

  if (dismissible)
    lv_obj_add_flag(popup, LV_OBJ_FLAG_USER_1);
  else
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_USER_1);
}
