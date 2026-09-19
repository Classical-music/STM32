#include "page_calendar.h"
#include "beep/task_beep.h"
#include "components/ui_popup.h"
#include "rtc/task_rtc.h"
#include "lvgl.h"
#include <stdio.h>

/* ==================== 布局常量 ====================
 * 页面内容区 432 x 636（480 x 684 减去四周 24 内边距），
 * 全部使用固定坐标，避免运行时布局计算开销。
 */
/* 顶部摘要卡片：今天日期 / 星期 / 时钟。 */
#define CAL_CARD_X 0
#define CAL_CARD_Y 0
#define CAL_CARD_WIDTH 432
#define CAL_CARD_HEIGHT 112
#define CAL_DATE_X 24
#define CAL_DATE_Y 16
#define CAL_WEEKDAY_X 24
#define CAL_WEEKDAY_Y 58
#define CAL_TIME_X 248
#define CAL_TIME_Y 16
#define CAL_TIME_WIDTH 160

/* 月份切换行：[<] 月份标签 [>]。 */
#define CAL_NAV_Y 124
#define CAL_NAV_HEIGHT 48
#define CAL_NAV_BTN_WIDTH 48
#define CAL_MONTH_LABEL_X 48
#define CAL_MONTH_LABEL_WIDTH 336

/* 星期表头与日期网格：7 列，单元格 60 x 56，横 2 竖 4 间距。 */
#define CAL_WEEKDAY_HEADER_Y 180
#define CAL_WEEKDAY_HEADER_HEIGHT 32
#define CAL_GRID_Y 218
#define CAL_CELL_WIDTH 60
#define CAL_CELL_HEIGHT 56
#define CAL_CELL_GAP_X 2
#define CAL_CELL_GAP_Y 4
#define CAL_CELL_STEP_X (CAL_CELL_WIDTH + CAL_CELL_GAP_X)
#define CAL_CELL_STEP_Y (CAL_CELL_HEIGHT + CAL_CELL_GAP_Y)
#define CAL_WEEK_COUNT 7
#define CAL_GRID_ROWS 6
#define CAL_CELL_COUNT (CAL_WEEK_COUNT * CAL_GRID_ROWS)

/* 回到本月按钮。 */
#define CAL_TODAY_X 146
#define CAL_TODAY_Y 586
#define CAL_TODAY_WIDTH 140
#define CAL_TODAY_HEIGHT 44

/* 时间设置弹窗：外壳由 ui_popup 组件提供，这里只定义主区域尺寸和内部布局。 */
#define POP_CARD_WIDTH 320
#define POP_CARD_HEIGHT 336
/* 弹窗字段行：[标签] [减号] [数值框] [加号]，左右边距对称。 */
#define POP_ROW_Y0 52
#define POP_ROW_STEP 62
#define POP_ROW_HEIGHT 44
#define POP_LABEL_X 28
#define POP_LABEL_WIDTH 64
#define POP_DEC_X 100
#define POP_STEP_BTN_WIDTH 48
#define POP_VALUE_X 156
#define POP_VALUE_WIDTH 80
#define POP_INC_X 244
#define POP_HINT_Y 232
#define POP_ACTION_Y 268
#define POP_ACTION_WIDTH 128
#define POP_SAVE_X 28
#define POP_CANCEL_X 164

/* ==================== 颜色 ==================== */
#define CAL_COLOR_BLUE 0x3A76F4
#define CAL_COLOR_BLUE_PRESSED 0x2E5FCC
#define CAL_COLOR_CARD_BG 0xFFFFFF
#define CAL_COLOR_TEXT_DARK 0x1A1A1A
#define CAL_COLOR_TEXT_MUTED 0xAEB4BC
#define CAL_COLOR_HEADER 0x3C4043
#define CAL_COLOR_PRESSED 0xE6E6E6
#define CAL_COLOR_ERROR 0xD93025

/* ==================== 显示文本表（默认字体仅含 ASCII，用英文） ========== */
static const char *const CAL_WEEKDAY_NAMES[CAL_WEEK_COUNT] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *const CAL_WEEKDAY_FULL[CAL_WEEK_COUNT] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"};
static const char *const CAL_MONTH_NAMES[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

/* 月份切换按钮动作。 */
enum {
  CAL_NAV_PREV = 0, /* 上一月 */
  CAL_NAV_NEXT,     /* 下一月 */
  CAL_NAV_TODAY,    /* 回到本月 */
};

/* 弹窗字段枚举，与时序 Hour / Min / Sec 对应。 */
enum {
  POP_FIELD_HOUR = 0,
  POP_FIELD_MIN,
  POP_FIELD_SEC,
  POP_FIELD_COUNT
};

static const char *const POP_FIELD_NAMES[POP_FIELD_COUNT] = {"Hour", "Min",
                                                             "Sec"};

/* ==================== 页面状态 ==================== */
/* 当前浏览的年月，进入页面时复位为真实当月。 */
static uint16_t g_view_year;
static uint8_t g_view_month;
/* 摘要刷新时记录的真实日期，跨天时定时器据此重绘网格。 */
static uint16_t g_last_year;
static uint8_t g_last_month;
static uint8_t g_last_day;

/* 页面级控件引用（销毁时全部置空）。 */
static lv_obj_t *g_page;
static lv_obj_t *g_date_label;
static lv_obj_t *g_weekday_label;
static lv_obj_t *g_time_label;
static lv_obj_t *g_month_label;
static lv_obj_t *g_day_cells[CAL_CELL_COUNT];
static lv_obj_t *g_day_labels[CAL_CELL_COUNT];
static lv_timer_t *g_update_timer;

/* 每个单元格对应的完整日期，render 时写入，点击单元格直接设置为该日期。 */
static uint16_t g_cell_year[CAL_CELL_COUNT];
static uint8_t g_cell_month[CAL_CELL_COUNT];
static uint8_t g_cell_day[CAL_CELL_COUNT];

/* 时间设置弹窗：外壳来自 ui_popup 组件，只构建一次后常驻隐藏复用。 */
static lv_obj_t *g_popup;
static lv_obj_t *g_pop_hint;
static lv_obj_t *g_pop_value_labels[POP_FIELD_COUNT];
static uint8_t g_pop_values[POP_FIELD_COUNT];

/* ==================== 日期算法 ==================== */

static bool calendar_is_leap(uint16_t year) {
  return (year % 4u == 0u) && ((year % 100u != 0u) || (year % 400u == 0u));
}

/* 公历年月日 -> 星期，Sakamoto 算法，返回值 0=周日。 */
static uint8_t calendar_weekday(uint16_t year, uint8_t month, uint8_t day) {
  static const uint8_t offsets[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  uint32_t y = year;

  if (month < 3)
    y -= 1;
  return (uint8_t)((y + y / 4 - y / 100 + y / 400 + offsets[month - 1] + day) %
                   7);
}

/* 某年某月的天数。 */
static uint8_t calendar_days_in_month(uint16_t year, uint8_t month) {
  static const uint8_t days[12] = {31, 28, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31};
  if (month == 2 && calendar_is_leap(year))
    return 29;
  return days[month - 1];
}

/* ==================== 共用样式 ====================
 * 所有自定义样式只在首次创建页面时构建一次，页面控件直接引用。
 * 日期单元格的三种形态（本月 / 邻月 / 今天）颜色在刷新时按单元格局部覆盖。
 */
enum {
  CAL_STYLE_CARD = 0,
  CAL_STYLE_NAV_BTN,
  CAL_STYLE_TODAY_BTN,
  CAL_STYLE_CELL,
  CAL_STYLE_HEADER_LABEL,
  CAL_STYLE_COUNT
};

static lv_style_t g_styles[CAL_STYLE_COUNT];
static bool g_styles_initialized;

static void calendar_styles_init(void) {
  if (g_styles_initialized)
    return;

  /* 顶部摘要卡片：白色圆角。 */
  lv_style_init(&g_styles[CAL_STYLE_CARD]);
  lv_style_set_bg_color(&g_styles[CAL_STYLE_CARD],
                        lv_color_hex(CAL_COLOR_CARD_BG));
  lv_style_set_bg_opa(&g_styles[CAL_STYLE_CARD], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[CAL_STYLE_CARD], 16);
  lv_style_set_border_width(&g_styles[CAL_STYLE_CARD], 0);
  lv_style_set_pad_all(&g_styles[CAL_STYLE_CARD], 0);
  lv_style_set_text_color(&g_styles[CAL_STYLE_CARD],
                          lv_color_hex(CAL_COLOR_TEXT_DARK));

  /* 月份切换按钮：白底描边。 */
  lv_style_init(&g_styles[CAL_STYLE_NAV_BTN]);
  lv_style_set_bg_color(&g_styles[CAL_STYLE_NAV_BTN],
                        lv_color_hex(CAL_COLOR_CARD_BG));
  lv_style_set_bg_opa(&g_styles[CAL_STYLE_NAV_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[CAL_STYLE_NAV_BTN], 10);
  lv_style_set_border_width(&g_styles[CAL_STYLE_NAV_BTN], 1);
  lv_style_set_border_color(&g_styles[CAL_STYLE_NAV_BTN],
                            lv_color_hex(0xC8C8C8));
  lv_style_set_pad_all(&g_styles[CAL_STYLE_NAV_BTN], 0);
  lv_style_set_text_color(&g_styles[CAL_STYLE_NAV_BTN],
                          lv_color_hex(CAL_COLOR_TEXT_DARK));

  /* 回到本月按钮：蓝底白字。 */
  lv_style_init(&g_styles[CAL_STYLE_TODAY_BTN]);
  lv_style_set_bg_color(&g_styles[CAL_STYLE_TODAY_BTN],
                        lv_color_hex(CAL_COLOR_BLUE));
  lv_style_set_bg_opa(&g_styles[CAL_STYLE_TODAY_BTN], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[CAL_STYLE_TODAY_BTN], 10);
  lv_style_set_border_width(&g_styles[CAL_STYLE_TODAY_BTN], 0);
  lv_style_set_pad_all(&g_styles[CAL_STYLE_TODAY_BTN], 0);
  lv_style_set_text_color(&g_styles[CAL_STYLE_TODAY_BTN], lv_color_white());

  /* 日期单元格基础形状，具体配色由刷新函数按单元格覆盖。 */
  lv_style_init(&g_styles[CAL_STYLE_CELL]);
  lv_style_set_bg_color(&g_styles[CAL_STYLE_CELL],
                        lv_color_hex(CAL_COLOR_CARD_BG));
  lv_style_set_bg_opa(&g_styles[CAL_STYLE_CELL], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[CAL_STYLE_CELL], 8);
  lv_style_set_border_width(&g_styles[CAL_STYLE_CELL], 0);
  lv_style_set_pad_all(&g_styles[CAL_STYLE_CELL], 0);
  lv_style_set_text_color(&g_styles[CAL_STYLE_CELL],
                          lv_color_hex(CAL_COLOR_TEXT_DARK));

  /* 星期表头：小号灰字（无更小字体可用，用灰色区分）。 */
  lv_style_init(&g_styles[CAL_STYLE_HEADER_LABEL]);
  lv_style_set_text_color(&g_styles[CAL_STYLE_HEADER_LABEL],
                          lv_color_hex(CAL_COLOR_HEADER));

  g_styles_initialized = true;
}

/* ==================== 控件构建辅助 ==================== */

/* 创建一个居中文本的小按钮，动作枚举通过 user_data 传入。 */
static lv_obj_t *calendar_create_btn(lv_obj_t *parent, const char *text,
                                     lv_coord_t x, lv_coord_t y,
                                     lv_coord_t w, lv_coord_t h,
                                     lv_style_t *style,
                                     lv_event_cb_t on_click, void *user_data) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_t *label = lv_label_create(button);

  lv_obj_remove_style_all(button);
  lv_obj_add_style(button, style, 0);
  lv_obj_set_size(button, w, h);
  lv_obj_set_pos(button, x, y);

  /* 主题样式清空后按下反馈丢失，用按下态背景色补回。 */
  lv_obj_set_style_bg_color(button, lv_color_hex(CAL_COLOR_PRESSED),
                            LV_STATE_PRESSED);

  lv_label_set_text_static(label, text);
  lv_obj_center(label);

  lv_obj_add_event_cb(button, on_click, LV_EVENT_CLICKED, user_data);
  return button;
}

/* ==================== 刷新逻辑 ==================== */

/* 重绘月历网格：6 行 7 列，首日按星期对齐，前后补邻月日期（灰显）。 */
static void calendar_render_month(void) {
  rtc_time_t now;
  uint8_t days, prev_days, lead;
  uint8_t i;
  char buf[8];

  rtcGetLocalTime(&now);

  days = calendar_days_in_month(g_view_year, g_view_month);
  prev_days = calendar_days_in_month(
      g_view_month == 1 ? (uint16_t)(g_view_year - 1) : g_view_year,
      g_view_month == 1 ? 12 : (uint8_t)(g_view_month - 1));
  lead = calendar_weekday(g_view_year, g_view_month, 1);

  for (i = 0; i < CAL_CELL_COUNT; i++) {
    lv_obj_t *cell = g_day_cells[i];
    lv_obj_t *label = g_day_labels[i];
    int16_t day = (int16_t)(i + 1) - (int16_t)lead; /* 1 起算的本月日序 */
    uint16_t cell_year = g_view_year;
    uint8_t cell_month = g_view_month;
    bool other_month = false;
    bool is_today = false;

    if (day < 1) {
      day = (int16_t)(prev_days + day); /* 上月末尾补位 */
      other_month = true;
      cell_year = g_view_month == 1 ? (uint16_t)(g_view_year - 1) : g_view_year;
      cell_month = g_view_month == 1 ? 12 : (uint8_t)(g_view_month - 1);
    } else if (day > days) {
      day = (int16_t)(day - days); /* 下月开头补位 */
      other_month = true;
      cell_year =
          g_view_month == 12 ? (uint16_t)(g_view_year + 1) : g_view_year;
      cell_month = g_view_month == 12 ? 1 : (uint8_t)(g_view_month + 1);
    } else if (g_view_year == now.year && g_view_month == now.month &&
               (uint8_t)day == now.day) {
      is_today = true;
    }

    /* 记录本格完整日期供点击设置使用；邻月补位格不可点击，防误触。 */
    g_cell_year[i] = cell_year;
    g_cell_month[i] = cell_month;
    g_cell_day[i] = (uint8_t)day;
    if (other_month)
      lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    else
      lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);

    snprintf(buf, sizeof(buf), "%d", day);
    lv_label_set_text(label, buf);

    /* 局部样式属性优先于共用样式，直接切换三种形态。 */
    if (other_month) {
      lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
      lv_obj_set_style_text_color(label, lv_color_hex(CAL_COLOR_TEXT_MUTED), 0);
    } else if (is_today) {
      lv_obj_set_style_bg_color(cell, lv_color_hex(CAL_COLOR_BLUE), 0);
      lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
    } else {
      lv_obj_set_style_bg_color(cell, lv_color_hex(CAL_COLOR_CARD_BG), 0);
      lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
      lv_obj_set_style_text_color(label, lv_color_hex(CAL_COLOR_TEXT_DARK), 0);
    }
  }

  snprintf(buf, sizeof(buf), "%s %04u", CAL_MONTH_NAMES[g_view_month - 1],
           g_view_year);
  lv_label_set_text(g_month_label, buf);
}

/* 刷新顶部摘要：今天的日期、星期和秒级时钟。 */
static void calendar_update_summary(void) {
  rtc_time_t now;
  char buf[24];

  rtcGetLocalTime(&now);

  snprintf(buf, sizeof(buf), "%04u-%02u-%02u", now.year, now.month, now.day);
  lv_label_set_text(g_date_label, buf);
  lv_label_set_text_static(g_weekday_label,
                           CAL_WEEKDAY_FULL[calendar_weekday(
                               now.year, now.month, now.day)]);

  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", now.hour, now.minute,
           now.second);
  lv_label_set_text(g_time_label, buf);

  /* 跨天时重绘网格，保证今天的蓝底高亮移动到新的日期格。 */
  if (now.year != g_last_year || now.month != g_last_month ||
      now.day != g_last_day) {
    g_last_year = now.year;
    g_last_month = now.month;
    g_last_day = now.day;
    calendar_render_month();
  }
}

/* 1 秒定时器：刷新时钟与日期，供缓存复用的页面保持实时。 */
static void calendar_timer_cb(lv_timer_t *timer) {
  (void)timer;
  if (g_date_label)
    calendar_update_summary();
}

/* ==================== 事件回调 ==================== */

static void calendar_nav_event_cb(lv_event_t *event) {
  lv_uintptr_t action = (lv_uintptr_t)lv_event_get_user_data(event);
  rtc_time_t now;

  switch (action) {
  case CAL_NAV_PREV:
    if (g_view_month == 1) {
      g_view_month = 12;
      g_view_year--;
    } else {
      g_view_month--;
    }
    break;

  case CAL_NAV_NEXT:
    if (g_view_month == 12) {
      g_view_month = 1;
      g_view_year++;
    } else {
      g_view_month++;
    }
    break;

  case CAL_NAV_TODAY:
  default:
    rtcGetLocalTime(&now);
    g_view_year = now.year;
    g_view_month = now.month;
    break;
  }

  calendar_render_month();
}

/* 点击日期格：保持当前时分秒，直接把日期设置为本格对应的年月日。 */
static void calendar_cell_event_cb(lv_event_t *event) {
  lv_uintptr_t idx = (lv_uintptr_t)lv_event_get_user_data(event);
  rtc_time_t t;

  rtcGetLocalTime(&t);
  t.year = g_cell_year[idx];
  t.month = g_cell_month[idx];
  t.day = g_cell_day[idx];

  if (rtcSetLocalTime(&t)) {
    playBeep(20);
    /* 摘要刷新检测到日期变化后会自动重绘网格，移动今天高亮。 */
    calendar_update_summary();
  }
}

/* ==================== 时间设置弹窗 ==================== */

static void timepop_render(void) {
  char buf[4];
  uint8_t field;

  for (field = 0; field < POP_FIELD_COUNT; field++) {
    snprintf(buf, sizeof(buf), "%02u", g_pop_values[field]);
    lv_label_set_text(g_pop_value_labels[field], buf);
  }
}

/* user_data = 字段序号 * 2 +（加号 ? 1 : 0），时圈 24、分秒圈 60。 */
static void timepop_step_event_cb(lv_event_t *event) {
  lv_uintptr_t code = (lv_uintptr_t)lv_event_get_user_data(event);
  uint8_t field = (uint8_t)(code / 2);
  int delta = (code & 1u) ? 1 : -1;
  int max = (field == POP_FIELD_HOUR) ? 24 : 60;

  g_pop_values[field] =
      (uint8_t)(((int)g_pop_values[field] + delta + max) % max);
  timepop_render();
}

static void timepop_save_event_cb(lv_event_t *event) {
  rtc_time_t t;

  (void)event;

  /* 保持当前日期不变，只提交弹窗里编辑的时分秒。 */
  rtcGetLocalTime(&t);
  t.hour = g_pop_values[POP_FIELD_HOUR];
  t.minute = g_pop_values[POP_FIELD_MIN];
  t.second = g_pop_values[POP_FIELD_SEC];

  if (rtcSetLocalTime(&t)) {
    playBeep(20);
    ui_popup_close(g_popup);
    calendar_update_summary();
    return;
  }

  lv_label_set_text_static(g_pop_hint, "Set failed, RTC not ready");
  lv_obj_set_style_text_color(g_pop_hint, lv_color_hex(CAL_COLOR_ERROR), 0);
}

static void timepop_cancel_event_cb(lv_event_t *event) {
  (void)event;
  ui_popup_close(g_popup);
}

/* 构建弹窗外壳与内部控件，只在首次打开时执行一次。 */
static void timepop_build(void) {
  lv_obj_t *card;
  lv_obj_t *label;
  uint8_t field;

  /* 遮罩 + 居中主区域由 ui_popup 组件提供，点遮罩空白处即关闭。 */
  g_popup = ui_popup_create(g_page, POP_CARD_WIDTH, POP_CARD_HEIGHT);
  if (!g_popup)
    return;

  card = ui_popup_get_body(g_popup);

  label = lv_label_create(card);
  lv_label_set_text_static(label, "Set Time");
  lv_obj_set_width(label, POP_CARD_WIDTH);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(label, 0, 14);

  for (field = 0; field < POP_FIELD_COUNT; field++) {
    lv_coord_t row_y = POP_ROW_Y0 + field * POP_ROW_STEP;
    lv_obj_t *box;

    label = lv_label_create(card);
    lv_label_set_text_static(label, POP_FIELD_NAMES[field]);
    lv_obj_set_width(label, POP_LABEL_WIDTH);
    lv_obj_set_pos(label, POP_LABEL_X,
                   row_y + (POP_ROW_HEIGHT - 24) / 2);

    calendar_create_btn(card, "-", POP_DEC_X, row_y, POP_STEP_BTN_WIDTH,
                        POP_ROW_HEIGHT, &g_styles[CAL_STYLE_NAV_BTN],
                        timepop_step_event_cb,
                        (void *)(lv_uintptr_t)(field * 2));

    box = lv_obj_create(card);
    lv_obj_remove_style_all(box);
    lv_obj_add_style(box, &g_styles[CAL_STYLE_NAV_BTN], 0);
    lv_obj_set_size(box, POP_VALUE_WIDTH, POP_ROW_HEIGHT);
    lv_obj_set_pos(box, POP_VALUE_X, row_y);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(box);
    lv_label_set_text_static(label, "00");
    lv_obj_center(label);
    g_pop_value_labels[field] = label;

    calendar_create_btn(card, "+", POP_INC_X, row_y, POP_STEP_BTN_WIDTH,
                        POP_ROW_HEIGHT, &g_styles[CAL_STYLE_NAV_BTN],
                        timepop_step_event_cb,
                        (void *)(lv_uintptr_t)(field * 2 + 1));
  }

  g_pop_hint = lv_label_create(card);
  lv_label_set_text_static(g_pop_hint, "");
  lv_obj_set_width(g_pop_hint, POP_CARD_WIDTH);
  lv_obj_set_style_text_align(g_pop_hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(g_pop_hint, 0, POP_HINT_Y);

  calendar_create_btn(card, "Save", POP_SAVE_X, POP_ACTION_Y,
                      POP_ACTION_WIDTH, POP_ROW_HEIGHT,
                      &g_styles[CAL_STYLE_TODAY_BTN], timepop_save_event_cb,
                      NULL);
  calendar_create_btn(card, "Cancel", POP_CANCEL_X, POP_ACTION_Y,
                      POP_ACTION_WIDTH, POP_ROW_HEIGHT,
                      &g_styles[CAL_STYLE_NAV_BTN], timepop_cancel_event_cb,
                      NULL);
}

/* 打开弹窗：用当前时间预填编辑值并清空提示。 */
static void timepop_open(void) {
  rtc_time_t now;

  if (!g_popup) {
    timepop_build();
    if (!g_popup)
      return;
  }

  rtcGetLocalTime(&now);
  g_pop_values[POP_FIELD_HOUR] = now.hour;
  g_pop_values[POP_FIELD_MIN] = now.minute;
  g_pop_values[POP_FIELD_SEC] = now.second;
  timepop_render();

  lv_label_set_text_static(g_pop_hint, "");
  lv_obj_set_style_text_color(g_pop_hint,
                              lv_color_hex(CAL_COLOR_TEXT_DARK), 0);
  ui_popup_open(g_popup);
}

/* 点击摘要卡片上的时间显示区：打开时间设置弹窗。 */
static void calendar_time_event_cb(lv_event_t *event) {
  (void)event;
  timepop_open();
}

/* ==================== 页面生命周期 ==================== */

/* 初始化日历页面。 */
void view_calendar_init(lv_obj_t *page, void *params) {
  rtc_time_t now;
  lv_obj_t *card;
  uint8_t i;

  (void)params;

  calendar_styles_init();

  g_page = page;

  /* 页面可能因缓存淘汰被重建，浏览月份复位为真实当月。 */
  rtcGetLocalTime(&now);
  g_view_year = now.year;
  g_view_month = now.month;
  g_last_year = now.year;
  g_last_month = now.month;
  g_last_day = now.day;

  /* 顶部摘要卡片。 */
  card = lv_obj_create(page);
  lv_obj_remove_style_all(card);
  lv_obj_add_style(card, &g_styles[CAL_STYLE_CARD], 0);
  lv_obj_set_size(card, CAL_CARD_WIDTH, CAL_CARD_HEIGHT);
  lv_obj_set_pos(card, CAL_CARD_X, CAL_CARD_Y);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  g_date_label = lv_label_create(card);
  lv_label_set_text_static(g_date_label, "0000-00-00");
  lv_obj_set_pos(g_date_label, CAL_DATE_X, CAL_DATE_Y);

  g_weekday_label = lv_label_create(card);
  lv_label_set_text_static(g_weekday_label, "Weekday");
  lv_obj_set_pos(g_weekday_label, CAL_WEEKDAY_X, CAL_WEEKDAY_Y);

  g_time_label = lv_label_create(card);
  lv_label_set_text_static(g_time_label, "00:00:00");
  lv_obj_set_width(g_time_label, CAL_TIME_WIDTH);
  lv_obj_set_style_text_align(g_time_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(g_time_label, CAL_TIME_X, CAL_TIME_Y);
  /* 点击时间文字打开时间设置弹窗。 */
  lv_obj_add_flag(g_time_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(g_time_label, calendar_time_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /* 月份切换行。 */
  calendar_create_btn(page, "<", 0, CAL_NAV_Y, CAL_NAV_BTN_WIDTH,
                      CAL_NAV_HEIGHT, &g_styles[CAL_STYLE_NAV_BTN],
                      calendar_nav_event_cb, (void *)(lv_uintptr_t)CAL_NAV_PREV);

  g_month_label = lv_label_create(page);
  lv_label_set_text_static(g_month_label, "MON 0000");
  lv_obj_set_width(g_month_label, CAL_MONTH_LABEL_WIDTH);
  lv_obj_set_style_text_align(g_month_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(g_month_label, CAL_MONTH_LABEL_X,
                 CAL_NAV_Y + (CAL_NAV_HEIGHT - 24) / 2);

  calendar_create_btn(page, ">",
                      CAL_MONTH_LABEL_X + CAL_MONTH_LABEL_WIDTH, CAL_NAV_Y,
                      CAL_NAV_BTN_WIDTH, CAL_NAV_HEIGHT,
                      &g_styles[CAL_STYLE_NAV_BTN], calendar_nav_event_cb,
                      (void *)(lv_uintptr_t)CAL_NAV_NEXT);

  /* 星期表头：7 列与网格列对齐。 */
  for (i = 0; i < CAL_WEEK_COUNT; i++) {
    lv_obj_t *label = lv_label_create(page);

    lv_obj_add_style(label, &g_styles[CAL_STYLE_HEADER_LABEL], 0);
    lv_label_set_text_static(label, CAL_WEEKDAY_NAMES[i]);
    lv_obj_set_width(label, CAL_CELL_WIDTH);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label, i * CAL_CELL_STEP_X, CAL_WEEKDAY_HEADER_Y);
  }

  /* 日期网格：6 行 x 7 列单元格，格内数字标签居中。 */
  for (i = 0; i < CAL_CELL_COUNT; i++) {
    lv_obj_t *cell = lv_obj_create(page);

    lv_obj_remove_style_all(cell);
    lv_obj_add_style(cell, &g_styles[CAL_STYLE_CELL], 0);
    lv_obj_set_size(cell, CAL_CELL_WIDTH, CAL_CELL_HEIGHT);
    lv_obj_set_pos(cell, (i % CAL_WEEK_COUNT) * CAL_CELL_STEP_X,
                   CAL_GRID_Y + (i / CAL_WEEK_COUNT) * CAL_CELL_STEP_Y);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    g_day_cells[i] = cell;
    g_day_labels[i] = lv_label_create(cell);
    lv_label_set_text_static(g_day_labels[i], "");
    lv_obj_center(g_day_labels[i]);

    /* 点击日期格直接把 RTC 日期设为该格（是否可点由 render 按本月/邻月控制）。 */
    lv_obj_add_event_cb(cell, calendar_cell_event_cb, LV_EVENT_CLICKED,
                        (void *)(lv_uintptr_t)i);
  }

  /* 回到本月按钮。 */
  calendar_create_btn(page, "Today", CAL_TODAY_X, CAL_TODAY_Y,
                      CAL_TODAY_WIDTH, CAL_TODAY_HEIGHT,
                      &g_styles[CAL_STYLE_TODAY_BTN], calendar_nav_event_cb,
                      (void *)(lv_uintptr_t)CAL_NAV_TODAY);

  calendar_render_month();
  calendar_update_summary();

  /* 秒级时钟定时器，随页面销毁一起删除。 */
  g_update_timer = lv_timer_create(calendar_timer_cb, 1000, NULL);
}

/*
 * 页面离开显示（转入缓存或压入历史栈）：暂停秒级时钟定时器，
 * 避免隐藏页面每秒白白刷新一遍控件。
 */
void view_calendar_hide(lv_obj_t *page) {
  (void)page;

  if (g_update_timer)
    lv_timer_pause(g_update_timer);
}

/*
 * 页面重新显示（缓存复用或从历史栈恢复）：恢复时钟定时器。
 * LVGL 在定时器 paused 期间不推进 last_run，离开超过一个周期时
 * 恢复后的第一轮 handler 会立即补发一次回调，时钟无需额外处理即可跟上。
 */
void view_calendar_show(lv_obj_t *page) {
  (void)page;

  if (g_update_timer)
    lv_timer_resume(g_update_timer);
}

/* 页面被缓存淘汰销毁前，清空本模块的控件引用和定时器，避免下次重建误判。 */
void view_calendar_destroy(lv_obj_t *page) {
  uint8_t i;

  (void)page;

  if (g_update_timer) {
    lv_timer_del(g_update_timer);
    g_update_timer = NULL;
  }

  g_date_label = NULL;
  g_weekday_label = NULL;
  g_time_label = NULL;
  g_month_label = NULL;
  /* 弹窗外壳是 page 的子对象，随页面一起销毁，这里只需置空引用等待下次重建。 */
  g_popup = NULL;
  g_pop_hint = NULL;
  for (i = 0; i < POP_FIELD_COUNT; i++) {
    g_pop_value_labels[i] = NULL;
  }
  g_page = NULL;
  for (i = 0; i < CAL_CELL_COUNT; i++) {
    g_day_cells[i] = NULL;
    g_day_labels[i] = NULL;
  }
}
