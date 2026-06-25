#include "app_led.h"
#include "timer.h"

// led 初始化
void app_led_init(void) {
  // 初始 LED 驱动
  led_init();
}

// ==================== 流水灯功能 begin ====================
// 流水灯上下文
typedef struct {
  uint8_t led_id;      // 当前ledId
  timer_id_t timer_id; // 流水周期定时器
} wl_ctx_t;
static wl_ctx_t wl_ctx = {LED_RED, -1};

static void wl_timeout_update_led(void *arg) {
  // 熄灭当前led
  led_off(wl_ctx.led_id);
  // 切换led
  wl_ctx.led_id = wl_ctx.led_id == LED_RED ? LED_GREEN : LED_RED;
  // 点亮新 led
  led_on(wl_ctx.led_id);
}

void water_light_start(void) {
  if (wl_ctx.timer_id != -1)
    return;             // 已启动流水灯
  breathe_light_stop(); // 停止呼吸灯

  // 初始化上下文
  wl_ctx.led_id = LED_RED;

  // 点亮第一个灯
  led_on(wl_ctx.led_id);

  // 启动定时器: 500ms, 循环
  wl_ctx.timer_id = TIMER_ADD_CB(500, wl_timeout_update_led, &wl_ctx, true);
}
void water_light_stop(void) {
  if (wl_ctx.timer_id != -1) {
    timer_del(wl_ctx.timer_id);
    wl_ctx.timer_id = -1;
  }

  // 关闭所有 LED
  led_off(LED_RED);
  led_off(LED_GREEN);
}
// ==================== 流水灯功能 end ====================

// ==================== 呼吸灯功能 begin ====================
typedef struct {
  uint8_t light;
  bool is_add;
  timer_id_t timer_id;
} bl_ctx_t;
static bl_ctx_t bl_ctx = {0, true, -1};

static void breathe_light_update_timer_cb(void *arg) {
  led_set_light(bl_ctx.light);
  if (bl_ctx.light == 0) {
    bl_ctx.is_add = true;
  } else if (bl_ctx.light == 99) {
    bl_ctx.is_add = false;
  }
  bl_ctx.light += bl_ctx.is_add ? 1 : -1;
}

void breathe_light_start(void) {
  if (bl_ctx.timer_id != -1)
    return; // 呼吸灯已启用

  water_light_stop(); // 首先关闭流水灯

  // ctx 初始化
  bl_ctx.light = 0;
  bl_ctx.is_add = true;
  bl_ctx.timer_id = -1;

  bl_ctx.timer_id = TIMER_ADD_CB(10, breathe_light_update_timer_cb, NULL, true);
}

void breathe_light_stop(void) {
  if (bl_ctx.timer_id != -1) {
    timer_del(bl_ctx.timer_id);
    bl_ctx.timer_id = -1;
  }
  led_off(LED_RED);
}
// ==================== 呼吸灯功能 end ====================
