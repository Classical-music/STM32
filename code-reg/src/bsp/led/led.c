#include "led.h"
#include "rcc.h"
#include "tim/tim.h"

// LED_RED - PB5
// LED_GREEN - PE5
#define RED_PORT GPIOB
#define RED_Pin 5
#define GREEN_PORT GPIOE
#define GREEN_Pin 5

void led_init(void) {
  // LED_GREEN, 普通灯
  RCC_GPIO_EN(RCC_GPIOE);
  GPIO_MODE_SET(GREEN_PORT, GREEN_Pin, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP);
  led_off(LED_GREEN);

  // LED_RED, PWM 灯
  tim_pwm_init();
  tim_pwm_start();
  led_off(LED_RED);
}

void led_on(uint8_t led) {
  switch (led) {
  case LED_RED:
    tim_pwm_dutyCycle(100);
    break;
  case LED_GREEN:
    GPIO_LOW(GREEN_PORT, GREEN_Pin);
    break;
  default:
    break;
  }
}
void led_off(uint8_t led) {
  switch (led) {
  case LED_RED:
    tim_pwm_dutyCycle(0);
    break;
  case LED_GREEN:
    GPIO_HIGH(GREEN_PORT, GREEN_Pin);
    break;
  default:
    break;
  }
}

void led_toggle(uint8_t led) {
  switch (led) {
  case LED_RED:
    // tim_pwm_dutyCycle(0);
    break;
  case LED_GREEN:
    GPIO_TOGGLE(GREEN_PORT, GREEN_Pin);
    break;
  default:
    break;
  }
}

// 仅支持 LED_RED
void led_set_light(uint8_t light) {
  tim_pwm_dutyCycle(light); // 设置亮度
}
