#ifndef TASK_LED_H
#define TASK_LED_H

#include <stdint.h>

/* 模拟器不连接真实 LED。
 * 这里提供与 STM32 端相同的接口，使共享页面代码无需区分平台。
 */
enum {
  LED_RED,
  LED_GREEN,
};

typedef enum {
  LED_MODE_ON,
  LED_MODE_OFF,
  LED_MODE_FLOW,
  LED_MODE_BREATH,
} led_mode_t;

static inline void ledTaskInit(void) {}
static inline void setLedMode(led_mode_t mode) { (void)mode; }
static inline void ledFlowlightSetInterval(uint32_t interval_ms) {
  (void)interval_ms;
}
static inline void ledBreathSetRange(uint8_t min_light, uint8_t max_light) {
  (void)min_light;
  (void)max_light;
}
static inline void led_on(uint8_t led) { (void)led; }
static inline void led_off(uint8_t led) { (void)led; }
static inline void led_set_light(uint8_t light) { (void)light; }

#endif
