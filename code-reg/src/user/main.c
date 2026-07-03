#include "led/led.h"
#include "task.h"

void vLEDTask(void *arg) {
  while (1) {
    led_on(LED_GREEN);
    vTaskDelay(500);
    led_off(LED_GREEN);
    vTaskDelay(500);
  }
}

int main(void) {
  // 硬件初始化
  led_init();

  // 创建 LED 任务
  BaseType_t xRet = xTaskCreate(vLEDTask, "LED", 128, NULL, 1, NULL);
  if (xRet != pdPASS) {
    led_on(LED_RED);
    while (1) {
    }
  } else {
    led_on(LED_GREEN);
  }

  // 启动调度器
  vTaskStartScheduler();

  while (1) {
  }
}
