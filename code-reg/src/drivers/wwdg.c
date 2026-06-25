#include "wwdg.h"
#include "rcc.h"

static uint8_t g_tr = 0x7F;

/**
 * 初始化窗口看门狗
 * tr: T[6:0], 计数器值
 * wr: W[6:0], 上窗口值
 * wdgtb: 分频系数
 * 取值范围 0x40 < wr <= tr <= 0x7F
 */
void wwdg_init(uint8_t tr, uint8_t wr, wwdg_wdgtb_t wdgtb) {
  tr &= 0x7F;
  if (wr <= 0x40 || tr < wr)
    return;
  g_tr = tr;

  RCC_WWDG_EN(RCC_WWDG);

  // 配置 W[6:0] & wdgtb
  WWDG->CFR = wdgtb | wr;
  WWDG->CR = tr; // 设置计数器 T[6:0]

  WWDG->CR |= WWDG_CR_WDGA; // 开启WWDG

  // NVIC
  NVIC_EnableIRQ(WWDG_IRQn);
  NVIC_SetPriority(WWDG_IRQn, IR_PRI_WWDG);

  WWDG->SR = 0;              // 清除EWIF标志位
  WWDG->CFR |= WWDG_CFR_EWI; // 使能 EWI 中断
}

void WWDG_IRQHandler(void) {
  if (WWDG->SR & WWDG_SR_EWIF) {
    WWDG->SR = 0;
    WWDG->CR |= g_tr & 0x7F;
  }
}
