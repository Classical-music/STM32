#include "gpio.h"
#include "i2c.h"
#include "tim/tim.h"
#include "touch.h"

/**
 * 引脚
 * INT - T_PEN - PF10 - 浮空输入
 * RST - T_CS - PF11 - 通用推挽输出
 * SCL - T_SLK - PB1 - 通用开漏输出
 * SDA - T_MOSI - PF9 - 通用开漏输出
 */
static i2c_ctx_t i2c_ctx = {
    .scl_port = GPIOB,
    .scl_pin = 1,
    .sda_port = GPIOF,
    .sda_pin = 9,
};

#define RESET(flag)                                                            \
  do {                                                                         \
    if (flag)                                                                  \
      GPIO_LOW(GPIOF, 11);                                                     \
    else                                                                       \
      GPIO_HIGH(GPIOF, 11);                                                    \
  } while (0)
static void gt9147_init(void) {
  // RCC
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPFEN;

  // gpio 模式
  GPIO_SET_MODES(GPIOF, GPIO_MODE_I | GPIO_CNF_I_Float, 10);
  GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP, 11);
  GPIO_SET_MODES(GPIOB, GPIO_MODE_O50 | GPIO_CNF_O_Gen_OD, 1);
  GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Gen_OD, 9);

  // 初始化 I2C
  I2C_INIT(i2c_ctx);

  RESET(true); // 复位
  delay_ms(10);
  RESET(false); // 释放复位
  delay_ms(1);
}

touch_t touch = {
    .init = gt9147_init,
    .scan_dir = NULL,
    .adjust = NULL,
};
