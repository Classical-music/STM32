/**
 * @file  bsp_seg.c
 * @brief 单只共阳极数码管实现：a~g 接 PA0~PA6，dp 接 PA7，低电平点亮
 */

#include "bsp_seg.h"

#include "gpio.h"

#define SEG_MASK    0x00FFu   /* 本模块只占用 PA0~PA7，写 BSRR 时屏蔽掉高 8 位 */

/* 十六进制段码表，下标 0~15 对应 0-F，bit0~bit7 对应 a~g/dp，1=点亮。
 * 'B'/'C'/'D' 的字形与 'b'/'c'/'d' 共用（数码管上大写形态更差），这是常规做法。 */
static const uint8_t s_hex_segs[16] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,  /* 0 1 2 3 4 5 6 7 */
    0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,  /* 8 9 A b C d E F */
};

/* 当前段码（逻辑值，1=点亮），由本模块维护，不回读引脚。
 * SEG_SetDot 需要在此基础上改一位，所以必须留影子状态。 */
static uint8_t s_segments = 0;

/* 把逻辑段码按共阳极极性取反后，用一次 BSRR 写入 PA0~PA7：
 * 低 16 位写 1 拉高（熄灭的段），高 16 位写 1 拉低（点亮的段）。
 * 单条写原子，8 段同时变化，不会出现半个字形的中间态，中断里调用也安全。 */
static void SEG_Write(uint8_t segments)
{
    s_segments = segments;

    SEG_PORT->BSRR = (uint32_t)((~segments) & SEG_MASK)
                   | (((uint32_t)segments & SEG_MASK) << 16u);
}

void SEG_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    GPIO_SET_MODE(SEG_PORT, GPIO_MODE_OUT_PP_2M, 0, 1, 2, 3, 4, 5, 6, 7);

    SEG_Off();
}

void SEG_DisplaySegments(uint8_t segments)
{
    SEG_Write(segments & SEG_MASK);
}

bool SEG_DisplayChar(char ch)
{
    uint8_t segs;

    if (ch >= '0' && ch <= '9')
    {
        segs = s_hex_segs[ch - '0'];
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        segs = s_hex_segs[ch - 'A' + 10];
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        segs = s_hex_segs[ch - 'a' + 10];
    }
    else if (ch == '-')
    {
        segs = SEG_G;
    }
    else if (ch == ' ')
    {
        segs = 0u;
    }
    else
    {
        return false;
    }

    /* 小数点保持原状态 */
    SEG_Write(segs | (s_segments & SEG_DP));

    return true;
}

void SEG_SetDot(bool on)
{
    if (on)
    {
        SEG_Write(s_segments | SEG_DP);
    }
    else
    {
        SEG_Write(s_segments & (uint8_t)~SEG_DP);
    }
}

void SEG_Off(void)
{
    SEG_Write(0u);
}
