/**
 * @file  test_beep.c
 * @brief 蜂鸣器自测序列实现：逐个音高播放，并探测 TIM3 的频率上下限
 *
 * PB0 现在是 TIM3_CH3 的硬件 PWM，响不响只说明通路通了，
 * 音高变了才说明 PSC/ARR 真的写进去了，所以这一轮全部围绕频率做用例。
 *
 * 三段内容：
 *   Scale  六个可听音高由低到高各响一段，用耳朵确认换频生效。
 *   Bounds 从 1Hz 起跳着往上试，头几个必然被 BEEP_SetTone() 拒掉（无声），
 *          越过 72MHz/65536 ≈ 1098Hz 之后才开始出声——听就是"前面空、后面响"，
 *          说明 16 位定时器的下限在这儿。末尾再试一次高于时钟的频率，应该同样无声。
 *   Hold   回到默认音高长响，然后静音，确认占空比归 0 时管子确实截止。
 * LED(PC13) 每个音高翻转一次，作为心跳指示程序还活着。
 *
 * 听不到声音时按这个顺序查：
 *   1. 万用表确认蜂鸣器(+)有 3.3V；
 *   2. 看 TIM3->PSC/ARR/CCR3/CCER/CR1：默认 2700Hz 时 PSC=0、ARR=26665、CCR3=13333
 *      （实际频率 72M/26666 ≈ 2699.7Hz，整数除法必然有这点偏差），
 *      CCER 里 CC3E=1 且 CC3P=0，CR1 里 CEN=1、ARPE=1；
 *   3. 前两点都对仍无声，多半是装了有源蜂鸣器——它内部自带振荡，
 *      认直流电平而不是方波，换频自然听不出来，需要按 On/Off 节奏驱动。
 */

#include "test_beep.h"

#include "board.h"

/* 节奏（毫秒），基准是 bsp/tick 的 1ms 节拍 */
#define BEEP_TONE_MS    250u   /* 每个音高的停留时间 */
#define BEEP_GAP_MS     120u   /* 音高之间的静音间隔 */
#define BEEP_HOLD_MS    800u   /* 末尾长响 */
#define BEEP_SILENT_MS  800u   /* 收尾静音的观察时间 */
#define BEEP_END_MS    1600u   /* 一轮结束的间隔 */

/* 音高阶梯（Hz）：从 TIM3 能凑出来的最低音一路往上，最后一档已经越过常见
 * 无源蜂鸣器的谐振点，用来确认高频段输出波形没有明显衰减。 */
static const uint32_t s_scale[] = { 1200u, 1600u, 2000u, 2700u, 4000u, 5800u };

/* 下限探测点（Hz）：1/100/500/1000 都在 1098 以下，应该全部失败；
 * 1100/1500 在范围内，应该出声。 */
static const uint32_t s_lower_probe[] = { 1u, 100u, 500u, 1000u, 1100u, 1500u };

/* 数组长度（gpio.h 里也有同名宏，这里不 include 驱动层，就地给一个测试用的） */
#define TONE_COUNT(arr)  (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief 响一个音高：先设频，成功才发声，失败就只留一段静音
 * @note  SetTone 失败时不会改动当前频率，所以这里必须自己判断返回值，
 *        否则会用上一个音高继续响，看起来像"频率没生效"。
 */
static void Test_BeepTone(uint32_t freq_hz, uint32_t hold_ms)
{
    if (BEEP_SetTone(freq_hz))
    {
        BEEP_On();
        TICK_DelayMs(hold_ms);
        BEEP_Off();
    }
    else
    {
        /* 频率超出 TIM3 能力，用两倍间隔留下可辨认的空档 */
        TICK_DelayMs(hold_ms + BEEP_GAP_MS);
    }

    LED_Toggle();
    TICK_DelayMs(BEEP_GAP_MS);
}

static void Test_BeepScale(void)
{
    uint32_t i;

    for (i = 0u; i < TONE_COUNT(s_scale); i++)
    {
        Test_BeepTone(s_scale[i], BEEP_TONE_MS);
    }
}

static void Test_BeepBounds(void)
{
    uint32_t i;

    for (i = 0u; i < TONE_COUNT(s_lower_probe); i++)
    {
        Test_BeepTone(s_lower_probe[i], BEEP_TONE_MS);
    }

    /* 高于 TIM3 计数时钟（72MHz），同样应该被拒掉 */
    Test_BeepTone(100000000u, BEEP_TONE_MS);
}

static void Test_BeepHold(void)
{
    BEEP_SetTone(BEEP_DEFAULT_HZ);
    BEEP_On();
    TICK_DelayMs(BEEP_HOLD_MS);
    BEEP_Off();
    LED_Toggle();
    TICK_DelayMs(BEEP_SILENT_MS);
}

void Beep_Test(void)
{
    Test_BeepScale();
    Test_BeepBounds();
    Test_BeepHold();

    TICK_DelayMs(BEEP_END_MS);
}
