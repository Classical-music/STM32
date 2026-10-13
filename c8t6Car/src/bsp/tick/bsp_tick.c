/**
 * @file  bsp_tick.c
 * @brief 系统节拍实现：直接操作内核 SysTick 寄存器，1ms 中断维护毫秒计数并提供忙等延时
 */

#include "bsp_tick.h"

/* LOAD 只有低 24 位有效，一个周期最多数 0xFFFFFF 个时钟 */
#define TICK_RELOAD_MAX    0x00FFFFFFu

/* 上电至今的毫秒数，只在中断里自增。
 * 32 位对齐读写在 Cortex-M3 上是单条指令，主流程读它不会读到半个值，
 * 所以 TICK_GetMs 不需要关中断。 */
static volatile uint32_t s_tick_ms = 0u;

void TICK_Init(void)
{
    uint32_t ticks = SystemCoreClock / TICK_PER_SEC;

    /* 24 位 LOAD 装不下就说明主频过高或 TICK_PER_SEC 过小，
     * 此时不启动计数，免得后面算出一个错的节拍周期却没人发现。 */
    if ((ticks == 0u) || (ticks > TICK_RELOAD_MAX))
    {
        return;
    }

    s_tick_ms = 0u;

    SysTick->LOAD = ticks - 1u;                       /* 两拍之间数 ticks 个时钟 */
    SysTick->VAL  = 0u;                               /* 清当前计数，顺带清 COUNTFLAG */
    NVIC_SetPriority(SysTick_IRQn, TICK_IRQ_PRIORITY);
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk        /* 用内核时钟 HCLK，不是 HCLK/8 */
                  | SysTick_CTRL_TICKINT_Msk          /* 计满进中断 */
                  | SysTick_CTRL_ENABLE_Msk;
}

uint32_t TICK_GetMs(void)
{
    return s_tick_ms;
}

void TICK_DelayMs(uint32_t ms)
{
    uint32_t start = TICK_GetMs();

    /* 用无符号差值比较，天然抗 49.7 天回绕 */
    while ((TICK_GetMs() - start) < ms)
    {
    }
}

/* startup_stm32f10x_md.s 里是 [WEAK] 定义，这里给出强符号覆盖它。
 * 一个工程只能有一个 SysTick_Handler：将来跑 RTOS 时要由 OS 接管这个向量。 */
void SysTick_Handler(void)
{
    s_tick_ms++;
}
