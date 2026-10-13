/**
 * @file  bsp_beep.h
 * @brief BSP 层蜂鸣器：PB0 = TIM3_CH3 硬件 PWM 输出
 *
 * 电路：BUZ1(+) 接 3.3V，BUZ1(-) 接 V1(SS8050, NPN) 集电极，发射极接 GND，
 *      PB0 -> R8 1.5KΩ -> 基极。PB0 高 -> 管子饱和导通 -> 蜂鸣器通电。
 *      属于低边开关，高电平有效，所以 PWM 用 active_high（CC3P=0）。
 *
 * 驱动方式：PB0 配成复用推挽，由 TIM3 通道 3 直接输出方波，不占用 CPU，
 *          也不需要软件翻转。频率就是音高，占空比固定 50%（方波声音最响）。
 *
 * 对上暴露"响/不响/换音高"三个语义，引脚、通道号、极性都封在本模块里。
 */

#ifndef __BSP_BEEP_H
#define __BSP_BEEP_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"

/* 蜂鸣器的物理位置；换板子改这里（通道号必须和引脚的 TIM3 默认映射对得上） */
#define BEEP_PORT     GPIOB
#define BEEP_PIN      0u                 /* PB0 */
#define BEEP_TIM_CH   TIM3_CH3           /* PB0 对应的 TIM3 通道 */

/* 上电默认音高（Hz）。无源蜂鸣器一般在 2~4kHz 最响，换料改这里 */
#define BEEP_DEFAULT_HZ   2700u

/**
 * @brief 初始化：使能 GPIOB 与 TIM3 时钟，把 PB0 配成复用推挽，定好默认频率，
 *        打开通道 3 的 PWM 输出并启动计数器，结束时处于静音状态
 * @note  驱动层（gpio.h / tim3.h）都不涉及 RCC，两个时钟都在这里使能；
 *        重复调用会重复开时钟，无副作用。
 * @note  本模块独占 TIM3。改频率的寄存器（PSC/ARR）是整块 TIM3 共用的，
 *        以后别的板载外设（比如 PB1 上的 CH4）也要用 TIM3 时，
 *        BEEP_SetTone() 会连带改掉那边的频率。
 */
void BEEP_Init(void);

/** @brief 响：按当前音高输出 50% 占空比的方波（单条 16 位写，中断里调用安全） */
void BEEP_On(void);

/** @brief 停：占空比归 0，引脚回到静音电平（同样是单条写，中断里安全） */
void BEEP_Off(void);

/** @brief 翻转，基于模块内部记录的状态，不回读引脚也不回读寄存器 */
void BEEP_Toggle(void);

/**
 * @brief 改音高（Hz），只在 BEEP_Init() 之后调用
 * @return 成功返回 true；freq_hz 越出 1098~72000 或 TIM3 时钟没开时返回 false，
 *         且不改动原来的频率
 * @note  下限是 72MHz / 65536（TIM3 是 16 位定时器，PSC 拉满也只能到这个频率）。
 * @note  内部是多次寄存器读改写，非原子，别在中断里调。
 * @note  改完 ARR 会立即生效，不产生半个周期的怪波形。
 */
bool BEEP_SetTone(uint32_t freq_hz);

/** @brief 读回当前音高（Hz），即 BEEP_SetTone() 最后一次成功设置的值 */
uint32_t BEEP_GetTone(void);

#endif /* __BSP_BEEP_H */
