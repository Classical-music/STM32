#ifndef __TASK_LED_H__
#define __TASK_LED_H__

#include "led/led.h"

// LED 灯模式
typedef enum {
  LED_MODE_NORMAL, // 普通模式
  LED_MODE_FLOW,   // 流水灯模式
  LED_MODE_BREATH, // 呼吸灯模式
} led_mode_t;

void ledInit(void);
void setLedMode(led_mode_t mode);

#endif
