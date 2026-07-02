#include "systick.h"

#if USE_FREERTOS == 0

void systick_init(uint16_t freq) {
  // 清空计数器&控制寄存器
  SysTick->CTRL = 0;
  SysTick->VAL = 0;

  // 设置重装值1ms
  SysTick->LOAD = SystemCoreClock / freq - 1;

  // 配置 CTRL
  SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk; // 内核时钟
  SysTick->CTRL |= SysTick_CTRL_TICKINT;       // 开中断
  SysTick->CTRL |= SysTick_CTRL_ENABLE;        // 启动 systick
}

__weak void hartbeat_handler(void) {}

// 中断处理程序
void SysTick_Handler(void) {
  SysTick->CTRL;      // 读取CTRL, 清零COUNTFLAG
  hartbeat_handler(); // 执行一次心跳
}

#endif
