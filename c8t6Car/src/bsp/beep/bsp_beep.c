/**
 * @file  bsp_beep.c
 * @brief 蜂鸣器实现：PB0 = TIM3_CH3，硬件 PWM 方波驱动 SS8050 低边开关
 *
 * 分工：tim3.h 只管时基和通道语义，不碰 RCC、不碰 GPIO；
 *      本文件负责开 GPIOB/TIM3 时钟、把 PB0 配成复用推挽、定极性。
 */

#include "bsp_beep.h"

#include "gpio.h"
#include "tim3.h"

/* 响的时候给 50% 占空比（方波，无源蜂鸣器最响），静音给 0%（恒无效电平） */
#define BEEP_DUTY_ON     50u
#define BEEP_DUTY_OFF     0u

/* 状态由本模块维护。TIM3->CCR3 是 16 位寄存器，单次写入在 Cortex-M3 上是原子的，
 * 所以 BEEP_On/Off/Toggle 可以在中断里调用；
 * BEEP_Init/SetTone 是多次寄存器读改写，只能在主流程里用。 */
static bool     s_beep_on = false;
static uint32_t s_tone_hz = BEEP_DEFAULT_HZ;

void BEEP_Init(void)
{
    /* 端口时钟 + 定时器时钟（TIM3 挂在 APB1 上） */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* PB0 交给 TIM3，先停掉通道，避免刚配成复用就吐出一个不确定的电平 */
    TIM3_DisablePwm(BEEP_TIM_CH);
    GPIO_SET_MODE(BEEP_PORT, GPIO_MODE_AF_PP_50M, BEEP_PIN);

    /* 先定频率（写 PSC/ARR），再配通道，最后开计数器 */
    if (TIM3_SetFreq(s_tone_hz) == 0u)
    {
        s_tone_hz = BEEP_DEFAULT_HZ;
        TIM3_SetFreq(s_tone_hz);
    }

    /* 低边开关 => 高电平有效，active_high 传非 0（CC3P=0） */
    TIM3_EnablePwm(BEEP_TIM_CH, 1u);
    *TIM3_Ccr(BEEP_TIM_CH) = 0u;     /* 初值 0：PWM 模式 1 下输出恒为无效电平 */

    TIM3_Start();

    s_beep_on = false;
}

void BEEP_On(void)
{
    *TIM3_Ccr(BEEP_TIM_CH) = (uint16_t)(((uint32_t)TIM3->ARR + 1u) * BEEP_DUTY_ON / 100u);
    s_beep_on = true;
}

void BEEP_Off(void)
{
    *TIM3_Ccr(BEEP_TIM_CH) = BEEP_DUTY_OFF;
    s_beep_on = false;
}

void BEEP_Toggle(void)
{
    if (s_beep_on)
    {
        BEEP_Off();
    }
    else
    {
        BEEP_On();
    }
}

bool BEEP_SetTone(uint32_t freq_hz)
{
    uint32_t period;

    /* TIM3_SetFreq() 自己会挡掉 0 和高于时钟的值，这里只管成功失败 */
    period = TIM3_SetFreq(freq_hz);
    if (period == 0u)
    {
        return false;
    }

    s_tone_hz = freq_hz;

    /* ARR 变了，占空比按新周期重算一次，否则响着的蜂鸣器音量会跟着变 */
    if (s_beep_on)
    {
        BEEP_On();
    }

    return true;
}

uint32_t BEEP_GetTone(void)
{
    return s_tone_hz;
}
