/**
 * @file  bsp_tick.h
 * @brief BSP 层系统节拍：SysTick 产生 1ms 中断，维护毫秒计数并提供忙等延时
 *
 * SysTick 是 Cortex-M3 内核外设，与板子接线无关，所以不设独立驱动层，
 * 本模块直接读写 LOAD/VAL/CTRL（寄存器位掩码来自 core_cm3.h）。
 * 时钟频率取自 SystemCoreClock（由 system_stm32f10x.c 按实际 PLL 配置算出），
 * 不硬编码 72000000，换主频不用改这里。
 *
 * @note 不要用 core_cm3.h 的 SysTick_Config(ticks)：它同样整体写 CTRL，
 *       且把优先级硬编码成最低，与本模块混用会互相覆盖，只保留这一条配置路径。
 */

#ifndef __BSP_TICK_H
#define __BSP_TICK_H

#include <stdint.h>
#include "stm32f10x.h"

/* 节拍频率：1000 即 1ms 一次中断 */
#define TICK_PER_SEC        1000u

/* SysTick 排最低优先级（本工程 4 位优先级，15 最低）：
 * 它只做一个自增，没有实时性要求，排最低可以避免抢占其它中断。
 * 以后加了别的 ISR，这个值就是它们的相对高低基准。 */
#define TICK_IRQ_PRIORITY   15u

/**
 * @brief 初始化 SysTick 为 1ms 周期中断，并把毫秒计数清零、开始计时
 * @note  要在其它外设初始化之前调用，后面的模块才可能用到 TICK_DelayMs。
 * @note  SysTick 属于内核，不受 RCC 管控，不需要开任何时钟；
 *        也不走 NVIC 的 ISER，只要 CTRL.TICKINT 置 1 就会进中断。
 * @note  CTRL.CLKSOURCE 固定取内核时钟 HCLK（不是 HCLK/8），
 *        下面的重装载值换算是按 HCLK 算的，改了这个位就得跟着改。
 * @note  主频过高时 1ms 的时钟数会超出 24 位 LOAD（上限约 233ms 一拍），
 *        本函数会直接返回不启动计数；这种情况要么把 TICK_PER_SEC 调小，
 *        要么改用 TIM 做节拍。本工程 72MHz 下是 72000 个时钟，余量充足。
 */
void TICK_Init(void);

/**
 * @brief 上电至今的毫秒数
 * @note  uint32_t 对齐读是原子的，中断里调用安全；约 49.7 天溢出回绕，
 *        做差值比较（如 TICK_DelayMs 内部）用无符号减法即可正确处理回绕。
 */
uint32_t TICK_GetMs(void);

/** @brief 忙等若干毫秒；要求全局中断是打开的（复位后默认打开），
 *         若在关中断的临界区里调用会死等。 */
void TICK_DelayMs(uint32_t ms);

#endif /* __BSP_TICK_H */
