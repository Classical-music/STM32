#ifndef __LED_H__
#define __LED_H__

#include "gpio.h"

enum {
  LED_RED,
  LED_GREEN,
};

void led_init(void);
void led_on(uint8_t led);
void led_off(uint8_t led);
void led_toggle(uint8_t led);

void led_set_light(uint8_t light);

#endif
