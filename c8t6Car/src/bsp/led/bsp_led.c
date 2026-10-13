/**
 * @file  bsp_led.c
 * @brief 板载 LED 实现：PC13，低电平点亮
 */

#include "bsp_led.h"

#include "gpio.h"

/* 当前点亮状态，由本模块维护；GPIO_HIGH/GPIO_LOW 走 BSRR/BRR 是原子写，
 * 中断里调用这几个接口也安全，但不要在 ISR 与主流程同时改同一个 LED。 */
static bool s_led_on = false;

void LED_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIO_SET_MODE(LED_PORT, GPIO_MODE_OUT_PP_2M, LED_PIN);

    LED_Off();
}

void LED_On(void)
{
    GPIO_LOW(LED_PORT, LED_PIN);
    s_led_on = true;
}

void LED_Off(void)
{
    GPIO_HIGH(LED_PORT, LED_PIN);
    s_led_on = false;
}

void LED_Toggle(void)
{
    if (s_led_on)
    {
        LED_Off();
    }
    else
    {
        LED_On();
    }
}
