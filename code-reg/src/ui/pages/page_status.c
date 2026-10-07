#include "page_status.h"
#include "adc/adc.h"
#include "light_sensor/light_sensor.h"
#include "lvgl.h"

/* ==================== 布局常量 ====================
 * 页面内容区 432 x 636（480 x 684 减去四周 24 内边距），
 * 三张信息卡片纵向排列，全部使用固定坐标。
 * 高度取 190：3 * 190 + 2 * 24 = 618，恰好留在 636 之内。
 */
#define STATUS_CARD_WIDTH 432
#define STATUS_CARD_HEIGHT 190
#define STATUS_CARD_GAP 24
#define STATUS_TEMP_CARD_Y 0
#define STATUS_VOLT_CARD_Y (STATUS_CARD_HEIGHT + STATUS_CARD_GAP)
#define STATUS_LIGHT_CARD_Y (STATUS_VOLT_CARD_Y + STATUS_CARD_HEIGHT + STATUS_CARD_GAP)

/* 卡片内部布局：标题在上，数值居中，说明在下，三行按 64px 等距分布。 */
#define STATUS_TITLE_X 24
#define STATUS_TITLE_Y 18
#define STATUS_VALUE_Y 82
#define STATUS_HINT_Y 146

/* 刷新周期：1s 足够跟上温度变化，又不至于让 ADC 轮询占用过多 CPU。 */
#define STATUS_REFRESH_PERIOD_MS 1000

/* ==================== 颜色 ==================== */
#define STATUS_COLOR_TEXT_DARK 0x1A1A1A
#define STATUS_COLOR_TEXT_HINT 0x8A8A8A
#define STATUS_COLOR_ACCENT_TEMP 0xE25555
#define STATUS_COLOR_ACCENT_VOLT 0x3A76F4
#define STATUS_COLOR_ACCENT_LIGHT 0xF0A020

/* ==================== 模块状态 ====================
 * ADC 为全设备共享外设，只需初始化一次；页面被缓存淘汰后重建时
 * 不重复执行上电与校准流程，故该标志在 destroy 中也不复位。 */
static bool g_adc_inited;

/* 页面控件引用（destroy 中清空，避免重建时误判控件已存在）。 */
static lv_obj_t *g_temp_value_label;
static lv_obj_t *g_volt_value_label;
static lv_obj_t *g_light_value_label;

/* 1s 软定时器：退出页面暂停、重新显示恢复、销毁时删除。 */
static lv_timer_t *g_status_timer;

/* ==================== 共用样式 ==================== */
enum {
  STATUS_STYLE_CARD = 0,
  STATUS_STYLE_TITLE,
  STATUS_STYLE_HINT,
  STATUS_STYLE_COUNT
};

static lv_style_t g_styles[STATUS_STYLE_COUNT];
static bool g_styles_initialized;

static void status_styles_init(void) {
  if (g_styles_initialized)
    return;

  /* 白色圆角卡片。 */
  lv_style_init(&g_styles[STATUS_STYLE_CARD]);
  lv_style_set_bg_color(&g_styles[STATUS_STYLE_CARD], lv_color_hex(0xFFFFFF));
  lv_style_set_bg_opa(&g_styles[STATUS_STYLE_CARD], LV_OPA_COVER);
  lv_style_set_radius(&g_styles[STATUS_STYLE_CARD], 16);
  lv_style_set_border_width(&g_styles[STATUS_STYLE_CARD], 0);
  lv_style_set_pad_all(&g_styles[STATUS_STYLE_CARD], 0);
  lv_style_set_text_color(&g_styles[STATUS_STYLE_CARD],
                          lv_color_hex(STATUS_COLOR_TEXT_DARK));

  /* 卡片标题。 */
  lv_style_init(&g_styles[STATUS_STYLE_TITLE]);
  lv_style_set_text_color(&g_styles[STATUS_STYLE_TITLE],
                          lv_color_hex(STATUS_COLOR_TEXT_DARK));

  /* 小字说明。 */
  lv_style_init(&g_styles[STATUS_STYLE_HINT]);
  lv_style_set_text_color(&g_styles[STATUS_STYLE_HINT],
                          lv_color_hex(STATUS_COLOR_TEXT_HINT));

  g_styles_initialized = true;
}

/* ==================== 数据采集与刷新 ====================
 * 两个内部通道都要求足够长的采样窗口（手册要求 >= 17.1 周期，
 * 这里直接用最长的 239.5 周期换取稳定读数）；两次转换合计
 * 约 (239.5 + 12.5) * 2 / 9MHz ≈ 56us，对 UI 无感知。
 * 光照通道由 light_sensor 模块内部完成去极值平均滤波。 */
static void status_update_values(void) {
  uint16_t raw;
  int16_t temp_x10;
  uint32_t mv;

  /* 通道 16: 内部温度传感器, adc_temp_x10 返回 0.1℃ 单位。 */
  raw = adc_read(ADC_CH_TEMP, ADC_SAMPLE_TIME_2395CYC);
  temp_x10 = adc_temp_x10(raw);
  if (g_temp_value_label != NULL) {
    /* 避免 %f（lv_printf 不保证支持浮点）：拆成整数部分 + 一位小数。 */
    int16_t abs_x10 = temp_x10 < 0 ? -temp_x10 : temp_x10;
    lv_label_set_text_fmt(g_temp_value_label, "%s%d.%d C",
                          temp_x10 < 0 ? "-" : "", abs_x10 / 10,
                          abs_x10 % 10);
  }

  /* 通道 17: 内部参考电压 VREFINT（典型 1.2V），按 VDDA=3300mV 折算。
   * 若该读数长期偏离 1200mV 左右，通常说明实际 VDD 并非标准 3.3V。 */
  raw = adc_read(ADC_CH_VREF, ADC_SAMPLE_TIME_2395CYC);
  mv = adc_to_mv(raw);
  if (g_volt_value_label != NULL)
    lv_label_set_text_fmt(g_volt_value_label, "%lu mV", (unsigned long)mv);

  /* PF8 光敏分压：电压越低表示光照越强，同一次采样同时给出百分比与 mV。 */
  if (g_light_value_label != NULL) {
    uint32_t light_mv = light_sensor_read_mv();
    lv_label_set_text_fmt(g_light_value_label, "%d %%  (%lu mV)",
                          light_sensor_mv_to_percent(light_mv),
                          (unsigned long)light_mv);
  }
}

/* lv_timer 回调：在 gui 任务的 lv_task_handler 上下文中执行。 */
static void status_timer_cb(lv_timer_t *timer) {
  (void)timer;
  status_update_values();
}

/* ==================== 页面构建 ==================== */

/* 创建一张信息卡片：标题 + 数值 + 说明，数值靠强调色区分层次，
 * 数值标签通过 value_label_out 带回给调用方保存引用。 */
static lv_obj_t *status_create_card(lv_obj_t *page, lv_coord_t y,
                                    const char *title, const char *hint,
                                    uint32_t value_color,
                                    lv_obj_t **value_label_out) {
  lv_obj_t *card = lv_obj_create(page);
  lv_obj_t *label;

  lv_obj_remove_style_all(card);
  lv_obj_add_style(card, &g_styles[STATUS_STYLE_CARD], 0);
  lv_obj_set_size(card, STATUS_CARD_WIDTH, STATUS_CARD_HEIGHT);
  lv_obj_set_pos(card, 0, y);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  label = lv_label_create(card);
  lv_obj_add_style(label, &g_styles[STATUS_STYLE_TITLE], 0);
  lv_label_set_text_static(label, title);
  lv_obj_set_pos(label, STATUS_TITLE_X, STATUS_TITLE_Y);

  label = lv_label_create(card);
  lv_obj_set_style_text_color(label, lv_color_hex(value_color), 0);
  lv_label_set_text_static(label, "--");
  lv_obj_set_pos(label, 0, STATUS_VALUE_Y);
  lv_obj_set_width(label, STATUS_CARD_WIDTH);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  *value_label_out = label;

  label = lv_label_create(card);
  lv_obj_add_style(label, &g_styles[STATUS_STYLE_HINT], 0);
  lv_label_set_text_static(label, hint);
  lv_obj_set_pos(label, STATUS_TITLE_X, STATUS_HINT_Y);

  return card;
}

/* 初始化设备状态页面。 */
void view_status_init(lv_obj_t *page, void *params) {
  (void)params;

  /* ADC 首次使用时上电 + 校准（共享外设，只做一次）。 */
  if (!g_adc_inited) {
    adc_init();
    g_adc_inited = true;
  }

  status_styles_init();

  /* 温度卡片：通道 16 内部温度传感器。 */
  status_create_card(page, STATUS_TEMP_CARD_Y, "Temperature",
                     "ADC1 IN16 - internal sensor", STATUS_COLOR_ACCENT_TEMP,
                     &g_temp_value_label);

  /* 电压卡片：通道 17 内部参考电压 VREFINT。 */
  status_create_card(page, STATUS_VOLT_CARD_Y, "Voltage",
                     "ADC1 IN17 - VREFINT 1.2V",
                     STATUS_COLOR_ACCENT_VOLT, &g_volt_value_label);

  /* 光照卡片：PF8 光敏分压节点，由 ADC3 IN6 采集。 */
  status_create_card(page, STATUS_LIGHT_CARD_Y, "Light",
                     "ADC3 IN6 - PF8 photoresistor",
                     STATUS_COLOR_ACCENT_LIGHT, &g_light_value_label);

  /* 先同步刷新一次，避免首秒显示占位符。 */
  status_update_values();

  /* 1s 周期软定时器负责后续实时更新。 */
  g_status_timer = lv_timer_create(status_timer_cb,
                                   STATUS_REFRESH_PERIOD_MS, NULL);
}

/* 页面离开显示：暂停刷新定时器，隐藏期间不再轮询 ADC。 */
void view_status_hide(lv_obj_t *page) {
  (void)page;

  if (g_status_timer != NULL)
    lv_timer_pause(g_status_timer);
}

/* 页面重新显示：恢复定时器（LVGL 会自动补发到期的回调，无需手动刷新）。 */
void view_status_show(lv_obj_t *page) {
  (void)page;

  if (g_status_timer != NULL)
    lv_timer_resume(g_status_timer);
}

/* 页面被缓存淘汰销毁：删除定时器（回调引用页面控件，必须先删），
 * 再清空控件引用；ADC 保持初始化状态，g_adc_inited 不复位。 */
void view_status_destroy(lv_obj_t *page) {
  (void)page;

  if (g_status_timer != NULL) {
    lv_timer_del(g_status_timer);
    g_status_timer = NULL;
  }
  g_temp_value_label = NULL;
  g_volt_value_label = NULL;
  g_light_value_label = NULL;
}
