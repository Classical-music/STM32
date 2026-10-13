/**
 * @file  bsp_led.h
 * @brief BSP 层板载 LED：PC13 控制，低电平点亮
 *
 * 对上只暴露开关量语义的四个接口，点亮/熄灭与电平极性由本模块内部转换，
 * 上层不需要知道 PC13 是高有效还是低有效。
 */

#ifndef __BSP_LED_H
#define __BSP_LED_H

#include <stdbool.h>
#include "stm32f10x.h"

/* 板载 LED 的物理位置；换板子只改这两个宏（极性在 bsp_led.c 里转换） */
#define LED_PORT     GPIOC
#define LED_PIN      13u

/**
 * @brief 初始化：使能 GPIOC 时钟，把 PC13 配成推挽输出，并默认熄灭
 * @note  PC13~PC15 由内部稳压管供电，驱动能力受限（灌电流约 3mA、速度上限 2MHz），
 *        所以用 2MHz 推挽输出而不是 50MHz。
 * @note  驱动层（gpio.h）不涉及 RCC，端口时钟在这里使能；重复调用会重复开时钟，无副作用。
 */
void LED_Init(void);

/** @brief 点亮（PC13 输出低电平） */
void LED_On(void);

/** @brief 熄灭（PC13 输出高电平） */
void LED_Off(void);

/** @brief 翻转，基于模块内部记录的状态，不回读引脚 */
void LED_Toggle(void);

#endif /* __BSP_LED_H */
