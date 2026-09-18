#include "task_app.h"
#include "beep/task_beep.h"
#include "cmd/task_cmd.h"
#include "console/task_console.h"
#include "gui/task_gui.h"
#include "key/task_key.h"
#include "led/task_led.h"
#include "rtc/task_rtc.h"
#include "touch/task_touch.h"
#include "task.h"
#include <stdio.h>

void vAppTask(void *arg) {
  KeyMsg_t xMsg;
  while (1) {
    if (xQueueReceive(xKeyQue, &xMsg, portMAX_DELAY) == pdFAIL)
      continue;
    switch (xMsg.keyId) {
    case KEY_UP:
      setLedMode(LED_MODE_BREATH);
      break;
    case KEY_DOWN:
      setLedMode(LED_MODE_FLOW);
      break;
    case KEY_LEFT:
      setLedMode(LED_MODE_ON);
      break;
    case KEY_RIGHT:
      setLedMode(LED_MODE_OFF);
      break;

    case KEY_TPAD:
      playBeep(20);
      led_toggle(LED_GREEN);
      break;

    default:
      break;
    }

    switch (xMsg.evtType) {
    case KEY_EVT_PRESSED:
      playBeep(20);
      break;
    case KEY_EVT_RELEASED:
      playBeep(200);
      break;
    case KEY_EVT_LONG_PRESSED:
      playBeep(40);
      break;
    case KEY_EVT_LONG_RELEASED:
      playBeep(400);
      break;
    case KEY_EVT_HOLD:
      playBeep(80);
      break;

    default:
      break;
    }
  }
}

void appTaskInit(void) {
  rtcTaskInit();     // 初始化 RTC 服务(以编译时间为基准)
  ledTaskInit();     // 创建 LED 子任务
  beepTaskInit();    // 创建 Beep 子任务
  keyTaskInit();     // 创建 Key 子任务
  taskConsoleInit(); // 创建 console 子任务
  cmdTaskInit();     // 创建 cmd 子任务
  touchTaskInit();   // 创建 touch 子任务
  taskGuiInit();     // 创建 gui 子任务

  xTaskCreate(vAppTask, "appTask", 128, NULL, 2, NULL); // 创建APP任务

  // 启动调度器
  vTaskStartScheduler();
}
