#include "gpio.h"
#include "i2c/i2c.h"
#include "tim/tim.h"
#include "touch.h"

/**
 * 引脚
 * INT - T_PEN - PF10 - 浮空输入
 */

// ============================== RST ==============================
// RST - T_CS - PF11 - 通用推挽输出
#define RST_INIT                                                               \
  do {                                                                         \
    RCC->APB2ENR |= RCC_APB2ENR_IOPFEN;                                        \
    GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP, 11);              \
  } while (0)

// 开启/关闭复位
#define RESET(flag)                                                            \
  do {                                                                         \
    if (flag)                                                                  \
      GPIO_LOW(GPIOF, 11);                                                     \
    else                                                                       \
      GPIO_HIGH(GPIOF, 11);                                                    \
  } while (0)

// ============================== INT ==============================
#include "led/led.h"
// INT - T_PEN - PF10 - 浮空输入 + 中断检测
static void INT_Init(void) {
  // RCC
  RCC->APB2ENR |= RCC_APB2ENR_IOPFEN | RCC_APB2ENR_AFIOEN;
  // 浮空输入
  GPIO_SET_MODES(GPIOF, GPIO_MODE_I | GPIO_CNF_I_Float, 10);

  // 映射 EXTI 中断到 PF10
  AFIO->EXTICR[2] &= ~AFIO_EXTICR3_EXTI10;
  AFIO->EXTICR[2] |= AFIO_EXTICR3_EXTI10_PF;

  // 中断
  EXTI->PR |= EXTI_PR_PR10;     // 清除可能得挂起标志
  EXTI->RTSR |= EXTI_RTSR_TR10; // 上升沿触发
  EXTI->FTSR |= EXTI_FTSR_TR10; // 下降沿触发
  EXTI->IMR |= EXTI_IMR_MR10;   // 使能中断

  // NVIC
  NVIC_SetPriority(EXTI15_10_IRQn, IR_PRI_TOUCH);
  NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler() {
  if (EXTI->PR & EXTI_PR_PR10) {
    EXTI->PR |= EXTI_PR_PR10;
    led_toggle(LED_GREEN);
  }
}

// ============================== I2C ==============================
/**
 * SCL - T_SLK - PB1 - 通用开漏输出
 * SDA - T_MOSI - PF9 - 通用开漏输出
 */
// static i2c_ctx_t i2c_ctx = {
//     .scl_port = GPIOB,
//     .scl_pin = 1,
//     .sda_port = GPIOF,
//     .sda_pin = 9,
// };

// static bool gt9147_i2c_init(void) {
//   return true; //
// }

static bool gt9147_init(void) {
  // RST
  RST_INIT;
  // INT
  INT_Init();

  // // RCC
  // RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  // // gpio 模式
  // GPIO_SET_MODES(GPIOB, GPIO_MODE_O50 | GPIO_CNF_O_Gen_OD, 1);
  // GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Gen_OD, 9);

  // // 初始化 I2C
  // I2C_INIT(i2c_ctx);

  // RESET(true); // 复位
  // delay_ms(10);
  // RESET(false); // 释放复位
  // delay_ms(1);
  return true;
}

touch_t touch = {
    .init = gt9147_init,
    .scan_dir = NULL,
    .adjust = NULL,
};
