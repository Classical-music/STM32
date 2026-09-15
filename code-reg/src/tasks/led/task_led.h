#ifndef __TASK_LED_H__
#define __TASK_LED_H__

#include "led/led.h"

// LED 灯模式
typedef enum {
  LED_MODE_ON,     // 常亮模式
  LED_MODE_OFF,    // 常暗
  LED_MODE_FLOW,   // 流水灯模式
  LED_MODE_BREATH, // 呼吸灯模式
} led_mode_t;

void ledTaskInit(void);
void setLedMode(led_mode_t mode);

// 设置流水灯的切换间隔（毫秒），对正在运行的流水灯立即生效
void ledFlowlightSetInterval(uint32_t interval_ms);

// 设置呼吸灯的亮度范围（百分比 0-100），对正在运行的呼吸灯立即生效
void ledBreathSetRange(uint8_t min_light, uint8_t max_light);

#endif
