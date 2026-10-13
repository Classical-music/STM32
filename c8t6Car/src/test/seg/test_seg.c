/**
 * @file  test_seg.c
 * @brief 数码管自测序列实现：逐段扫描 -> 字符表 -> 小数点 -> 全亮/全灭
 *
 * 现象对照：
 *   Scan       每次只亮一段，顺序 a b c d e f g dp。哪一段不亮=那根线/那个焊点的问题；
 *              亮的位置和预期错位=接线与 SEG_A~SEG_DP 的位序对不上。
 *   Char       0123456789ABCDEF 后接 '-'、空格；末尾的 'G'/'.' 不在段码表内，
 *              接口返回 false，此时只翻转 LED、显示保持不变，用来验证返回值可用。
 *   Dot        显示 '3' 并闪小数点，验证 SEG_SetDot 不动其余段。
 *   AllOn/Off  8 段同时全亮再全灭，检查驱动能力与断段。
 * LED(PC13) 每走一步翻转一次，作为心跳指示程序还活着。
 */

#include "test_seg.h"

#include "board.h"

/* 节奏（毫秒），基准是 bsp/tick 的 1ms 节拍 */
#define T_HOLD    400u    /* 一步的驻留时间 */
#define T_HALF    200u    /* 半拍，用于闪烁 */
#define T_LONG    800u    /* 需要看清结果时 */
#define T_GAP    1600u    /* 一轮结束的间隔 */

static void Test_Scan(void)
{
    static const uint8_t steps[8] = { SEG_A, SEG_B, SEG_C, SEG_D,
                                      SEG_E, SEG_F, SEG_G, SEG_DP };

    for (uint32_t i = 0; i < 8u; i++)
    {
        SEG_DisplaySegments(steps[i]);
        LED_Toggle();
        TICK_DelayMs(T_HOLD);
    }
}

static void Test_Char(void)
{
    static const char text[] = "0123456789ABCDEF- G.";

    for (uint32_t i = 0; text[i] != '\0'; i++)
    {
        if (SEG_DisplayChar(text[i]))
        {
            LED_Toggle();
        }
        else
        {
            /* 表外字符：接口不改显示，这里用连续两次翻转 LED 表示"跳过" */
            LED_Toggle();
            LED_Toggle();
        }

        TICK_DelayMs(T_HALF);
    }
}

static void Test_Dot(void)
{
    SEG_DisplayChar('3');

    for (uint32_t i = 0; i < 6u; i++)
    {
        SEG_SetDot(true);
        TICK_DelayMs(T_HALF);
        SEG_SetDot(false);
        TICK_DelayMs(T_HALF);
    }
}

static void Test_AllOn(void)
{
    SEG_DisplaySegments(SEG_A | SEG_B | SEG_C | SEG_D |
                        SEG_E | SEG_F | SEG_G | SEG_DP);
    LED_Toggle();
    TICK_DelayMs(T_LONG);
}

static void Test_AllOff(void)
{
    SEG_Off();
    LED_Toggle();
    TICK_DelayMs(T_LONG);
}

void Seg_Test(void)
{
    Test_Scan();
    Test_Char();
    Test_Dot();
    Test_AllOn();
    Test_AllOff();

    TICK_DelayMs(T_GAP);
}
