#include "task_touch.h"
#include "task.h"
#include "touch/touch.h"

SemaphoreHandle_t xTouchSemaph = NULL;

static bool is_pressed = false;
static uint16_t pos_x;
static uint16_t pos_y;

void vTaskTouch(void *arg) {
  // 创建二值信号量
  xTouchSemaph = xSemaphoreCreateBinary();
  if (xTouchSemaph == NULL)
    return;

  while (1) {
    // 等待中断信号量
    if (xSemaphoreTake(xTouchSemaph, portMAX_DELAY) != pdTRUE)
      continue;

    // 调用 touch 驱动, 读取触摸状态&坐标
    is_pressed = touch_read(&pos_x, &pos_y);
  }
}

void taskTouchInit(void) {
  xTaskCreate(vTaskTouch, "lcd-touch", 1, NULL, 2, NULL);
}

bool taskTouchIsPressed(void) {
  return is_pressed; //
}

void taskTouchGetPos(uint16_t *x, uint16_t *y) {
  *x = pos_x;
  *y = pos_y;
}
