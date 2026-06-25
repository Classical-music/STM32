#ifndef __GPIO_H__
#define __GPIO_H__

#include "type.h"

typedef enum {
  GPIO_MODE_I,   // 输入
  GPIO_MODE_O10, // 输出 10M
  GPIO_MODE_O2,  // 输出 20M
  GPIO_MODE_O50, // 输出 50M
} gpio_mode_t;

typedef enum {
  GPIO_CNF_I_Ana = 0,   // 模拟输入
  GPIO_CNF_I_Float = 4, // 浮空输入
  GPIO_CNF_I_Pull = 8,  // 上/下拉输入

  GPIO_CNF_O_Gen_PP = 0,   // 通用推挽输出
  GPIO_CNF_O_Gen_OD = 4,   // 通用开漏输出
  GPIO_CNF_O_Res_PP = 8,   // 复用推挽输出
  GPIO_CNF_O_Res_OD = 0xC, // 复用开漏输出
} gpio_cnf_t;

#define GPIO_MODE_SET(port, pin, val)                                          \
  do {                                                                         \
    if (pin < 0)                                                               \
      break;                                                                   \
    uint8_t shift;                                                             \
    if (pin < 8) {                                                             \
      shift = (uint8_t)(pin << 2);                                             \
      port->CRL &= ~(0xF << shift);                                            \
      port->CRL |= (val) << shift;                                             \
    } else if (pin < 16) {                                                     \
      shift = (uint8_t)((pin - 8) << 2);                                       \
      port->CRH &= ~(0xF << shift);                                            \
      port->CRH |= (val) << shift;                                             \
    }                                                                          \
  } while (0)

#define GPIO_HIGH(port, pin)                                                   \
  do {                                                                         \
    if (pin < 0)                                                               \
      break;                                                                   \
    if (pin < 16) {                                                            \
      port->ODR |= 1 << pin;                                                   \
    }                                                                          \
  } while (0)

#define GPIO_LOW(port, pin)                                                    \
  do {                                                                         \
    if (pin < 0)                                                               \
      break;                                                                   \
    if (pin < 16) {                                                            \
      port->ODR &= ~(1 << pin);                                                \
    }                                                                          \
  } while (0)

#define GPIO_TOGGLE(port, pin)                                                 \
  do {                                                                         \
    if (pin < 0)                                                               \
      break;                                                                   \
    if (pin < 16)                                                              \
      port->ODR ^= (1 << pin);                                                 \
  } while (0)

#define GPIO_READ(port, pin) (port->IDR & (GPIO_IDR_IDR0 << pin))

#endif
