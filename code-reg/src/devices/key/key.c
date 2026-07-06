#include "key.h"
#include "gpio.h"
#include "rcc.h"
#include "tim.h"

// keyup - pa0
#define KEY_UP_Port GPIOA
#define KEY_UP_Pin 0

// keydown - pe3
#define KEY_DOWN_Port GPIOE
#define KEY_DOWN_Pin 3

// keyleft - pe2
#define KEY_LEFT_Port GPIOE
#define KEY_LEFT_Pin 2

// keyright - pe4
#define KEY_RIGHT_Port GPIOE
#define KEY_RIGHT_Pin 4

// 配置 下拉输入
#define GPIO_KEY_LOW(port, pin)                                                \
  GPIO_MODE_SET(port, pin, GPIO_MODE_I | GPIO_CNF_I_Pull);                     \
  GPIO_LOW(port, pin);
// 配置 上拉输入
#define GPIO_KEY_HIGH(port, pin)                                               \
  GPIO_MODE_SET(port, pin, GPIO_MODE_I | GPIO_CNF_I_Pull);                     \
  GPIO_HIGH(port, pin);

void key_init(void) {
  RCC_GPIO_EN(RCC_GPIOA | RCC_GPIOE);

  // 配置 上/下拉输入 & 默认下拉
  GPIO_KEY_LOW(KEY_UP_Port, KEY_UP_Pin);
  GPIO_KEY_HIGH(KEY_DOWN_Port, KEY_DOWN_Pin);
  GPIO_KEY_HIGH(KEY_LEFT_Port, KEY_LEFT_Pin);
  GPIO_KEY_HIGH(KEY_RIGHT_Port, KEY_RIGHT_Pin);
}
// 判断是否按下
bool key_ispress(uint8_t key) {
  bool ret = false;
  switch (key) {
  case KEY_UP:
    ret = !!GPIO_READ(KEY_UP_Port, KEY_UP_Pin);
    break;
  case KEY_DOWN:
    ret = !GPIO_READ(KEY_DOWN_Port, KEY_DOWN_Pin);
    break;

  case KEY_LEFT:
    ret = !GPIO_READ(KEY_LEFT_Port, KEY_LEFT_Pin);
    break;

  case KEY_RIGHT:
    ret = !GPIO_READ(KEY_RIGHT_Port, KEY_RIGHT_Pin);
    break;
  }
  return ret;
}
