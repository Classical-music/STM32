#include "task_beep.h"
#include "beep/beep.h"
#include "timers.h"

static TimerHandle_t xBeepTimer = NULL;

static void vBeepTimerHandler(TimerHandle_t xTimer) {
  beep_off(); //
}

void beepTaskInit(void) {
  xBeepTimer = xTimerCreate("beepTimer", 100, pdFALSE, NULL, vBeepTimerHandler);
}

void playBeep(uint32_t period_ms) {
  beep_on(); // 开启beep

  // 修改定时时间
  xTimerChangePeriod(xBeepTimer, period_ms / portTICK_PERIOD_MS,
                     pdMS_TO_TICKS(100));
  // 启动单发定时器
  xTimerReset(xBeepTimer, pdMS_TO_TICKS(100));
}
