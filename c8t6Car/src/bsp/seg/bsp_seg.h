/**
 * @file  bsp_seg.h
 * @brief BSP 层单只共阳极数码管：PA0~PA6 接 a~g，PA7 接 dp
 *
 * 接线（段码位与引脚号一一对应，低 8 位即整个数码管）：
 *   bit0=a/PA0  bit1=b/PA1  bit2=c/PA2  bit3=d/PA3
 *   bit4=e/PA4  bit5=f/PA5  bit6=g/PA6  bit7=dp/PA7
 *
 * 对上只暴露"段位/字符"语义的接口，共阳极的低电平有效极性在本模块内部转换，
 * 上层传进来的 segments 一律按 1=点亮 理解。
 */

#ifndef __BSP_SEG_H
#define __BSP_SEG_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"

/* 数码管的物理位置；换板子只改这个宏（极性在 bsp_seg.c 里转换）。
 * 前提：a~g/dp 必须连在同一个端口的 pin0~pin7 上，否则要连位序一起改。 */
#define SEG_PORT     GPIOA

/* 段位定义，供 SEG_DisplaySegments() 拼装自定义图形：
 * SEG_DisplaySegments(SEG_A | SEG_B | SEG_DP); */
#define SEG_A        0x001
#define SEG_B        0x002
#define SEG_C        0x004
#define SEG_D        0x008
#define SEG_E        0x010
#define SEG_F        0x020
#define SEG_G        0x040
#define SEG_DP       0x080

/**
 * @brief 初始化：使能 GPIOA 时钟，把 PA0~PA7 配成推挽输出，并默认全灭
 * @note  共阳极时 MCU 引脚是灌电流侧，串阻建议 330Ω~1kΩ（单段约 2~7mA，
 *        8 段同时点亮时总电流要留余量）。
 * @note  PA0~PA7 同时是 ADC1/2 的 IN0~IN7 和 USART2 的引脚，占用前先确认没有复用冲突。
 * @note  驱动层（gpio.h）不涉及 RCC，端口时钟在这里使能；重复调用会重复开时钟，无副作用。
 */
void SEG_Init(void);

/** @brief 原始段位控制：bit0~bit7 对应 a~g 与 dp，1 表示点亮，8 段一次 BSRR 写完，无中间态 */
void SEG_DisplaySegments(uint8_t segments);

/**
 * @brief 显示单个字符：'0'~'9'、'A'~'F'、'a'~'f'、'-'、' '
 * @return true 表示字符在段码表内并已显示；false 表示不支持，输出不变
 * @note  小数点不受影响，保持上一次的状态；需要显示 '3.' 这类带点字符，
 *        先调本函数再调 SEG_SetDot(true)。
 */
bool SEG_DisplayChar(char ch);

/** @brief 单独点亮/熄灭小数点，其余段保持当前显示 */
void SEG_SetDot(bool on);

/** @brief 全灭 */
void SEG_Off(void);

#endif /* __BSP_SEG_H */
