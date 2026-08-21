#include "task_touch.h"
#include "touch/bsp_touch.h"
#include "task.h"

static TaskHandle_t touch_task_handle = NULL;

static void touchIrqCallback(void) {
  BaseType_t task_woken = pdFALSE;

  // 通知 touch 任务有中断
  vTaskNotifyGiveFromISR(touch_task_handle, &task_woken);
  // 切换到 touch 任务处理中断
  portYIELD_FROM_ISR(task_woken);
}

// touch 任务函数
static void touchTask(void *arg) {
  (void)arg;

  while (1) {
    // 等待中断通知
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // 处理中断
    touch_process();
  }
}

void touchTaskInit(void) {
  // 创建 touch 任务
  if (xTaskCreate(touchTask, "touch", 256, NULL, 3, &touch_task_handle) == pdPASS) {
    // 注册中断回调函数
    touch_set_irq_callback(touchIrqCallback);
  }
}
