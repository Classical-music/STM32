#include "task_led.h"
#include "task.h"
#include "timers.h"

// ==================== 流水灯功能 begin ====================
// 流水灯上下文
typedef struct {
  uint8_t led_id;        // 当前ledId
  TimerHandle_t timer;   // 流水灯 timer
  uint32_t interval_ms;  // 切换间隔，默认 500ms
} flowlight_ctx_t;
static flowlight_ctx_t fl_ctx = {LED_RED, NULL, 500};

static void vFlowlightTimerHandler(TimerHandle_t xTimer) {
  // 熄灭当前 led
  led_off(fl_ctx.led_id);
  // 切换led
  fl_ctx.led_id = fl_ctx.led_id == LED_RED ? LED_GREEN : LED_RED;
  // 点亮新 led
  led_on(fl_ctx.led_id);
}

static void ledFlowlightInit(void) {
  // 创建流水灯timer
  fl_ctx.timer =
      xTimerCreate("flowLightTimer", 500, pdTRUE, 0, vFlowlightTimerHandler);
}

static void ledFlowlightStart(void) {
  // 初始化上下文
  fl_ctx.led_id = LED_RED;

  // 按当前设置的间隔更新定时器周期
  xTimerChangePeriod(fl_ctx.timer, pdMS_TO_TICKS(fl_ctx.interval_ms),
                     pdMS_TO_TICKS(100));

  // 点亮第一个灯
  led_on(fl_ctx.led_id);

  // 启动定时器
  xTimerReset(fl_ctx.timer, pdMS_TO_TICKS(100));
}

static void ledFlowlightStop(void) {
  // 停止定时器
  xTimerStop(fl_ctx.timer, pdMS_TO_TICKS(100));
  // 关闭所有 led
  led_off(LED_RED);
  led_off(LED_GREEN);
}

// 设置流水灯的切换间隔（毫秒），对正在运行的流水灯立即生效
void ledFlowlightSetInterval(uint32_t interval_ms) {
  fl_ctx.interval_ms = interval_ms;

  if (fl_ctx.timer != NULL)
    xTimerChangePeriod(fl_ctx.timer, pdMS_TO_TICKS(interval_ms),
                       pdMS_TO_TICKS(100));
}

// ==================== 流水灯功能 end ====================

// ==================== 呼吸灯功能 begin ====================
static TaskHandle_t xBreathTaskHandle = NULL;
// 呼吸灯亮度范围（百分比），由 UI 设置，任务中读取
static volatile uint8_t breathMinLight = 0;
static volatile uint8_t breathMaxLight = 100;

void ledBreathSetRange(uint8_t min_light, uint8_t max_light) {
  breathMinLight = min_light;
  breathMaxLight = max_light;
}

void vBreathLedTask(void *arg) {
  uint16_t light = 0; // 当前亮度
  int8_t step = 1;    // 步进方向
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1) {
    // 固定10m刷新周期
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

    uint8_t light_min = breathMinLight;
    uint8_t light_max = breathMaxLight;

    // 范围被调窄到当前亮度之外时，直接回到范围内
    if (light < light_min)
      light = light_min;
    else if (light > light_max)
      light = light_max;

    // 更新亮度
    led_set_light(light);

    // 三角波步进 min -> max -> min
    light += step;
    if (light >= light_max) {
      light = light_max;
      step = -1;
    } else if (light <= light_min) {
      light = light_min;
      step = 1;
    }
  }
}

static void ledBreathStart(void) {
  if (xBreathTaskHandle == NULL) {
    // 创建呼吸灯任务
    xTaskCreate(vBreathLedTask, "breathLed", 128, NULL, 2, &xBreathTaskHandle);
    return;
  }
  vTaskResume(xBreathTaskHandle);
}
static void ledBreathStop(void) {
  if (xBreathTaskHandle != NULL) {
    // 先关闭LED
    led_off(LED_RED);

    // 停止任务
    vTaskSuspend(xBreathTaskHandle);
  }
}
// ==================== 呼吸灯功能 end ====================

// led 初始化
static bool isInit = false;
void ledTaskInit(void) {
  if (isInit)
    return;
  isInit = true;
  // 流水灯-初始化
  ledFlowlightInit();
}

void setLedMode(led_mode_t mode) {
  switch (mode) {
  case LED_MODE_ON:
    ledFlowlightStop();
    ledBreathStop();
    led_on(LED_RED);
    led_on(LED_GREEN);
    break;

  case LED_MODE_OFF:
    ledFlowlightStop();
    ledBreathStop();
    break;

  case LED_MODE_FLOW:
    ledBreathStop();
    ledFlowlightStart();
    break;

  case LED_MODE_BREATH:
    ledFlowlightStop();
    ledBreathStart();
    break;

  default:
    break;
  }
}
