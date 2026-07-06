#include "bsp_sram.h"
#include "fsmc.h"
#include "gpio.h"

/**
 * 初始化 sram
 * GPIO 引脚
 * A: PF15:12 PF5:0 PG5:0 PD:13:11
 * D: PD15:14 10:8 1:0 PE15:7
 *
 * UB - PE1 - NBL1
 * LB - PE0 - NBL0
 * OE - PD4 - NOE
 * WE - PD5 - NWE
 * CE - PG10 - NE3
 */
void xm8a5121_init(void) {
  // RCC
  RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN | RCC_APB2ENR_IOPFEN |
                  RCC_APB2ENR_IOPGEN;
  // GPIO
  // ADDR
  GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 15, 14, 13, 12, 5, 4,
                 3, 2, 1, 0);
  GPIO_SET_MODES(GPIOG, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 5, 4, 3, 2, 1, 0);
  GPIO_SET_MODES(GPIOD, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 13, 12, 11);
  // DATA
  GPIO_SET_MODES(GPIOD, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 15, 14, 10, 9, 8, 1,
                 0);
  GPIO_SET_MODES(GPIOE, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 15, 14, 13, 12, 11,
                 10, 9, 8, 7);

  fsmc_ne4_init();
}

static bsp_sram_t bsp_sram = {
    .init = NULL,
};