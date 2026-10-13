/**
 * @file  tim3.h
 * @brief TIM3 PWM 驱动（纯 static inline，直接操作寄存器，无 .c 文件，不依赖标准外设库）
 *
 * 放 drivers 层是因为这一段代码与板子接线无关：只管 TIM3 自身的时基和
 * 四个比较通道的 PWM 语义。引脚怎么配、极性取反、时钟使能这些和板子相关的事，
 * 由上层 bsp 决定。
 *
 * 对外接口：
 *   TIM3_ClockHz()                   读回 TIM3 实际的计数时钟（Hz）
 *   TIM3_SetFreq(freq_hz)            配 PSC/ARR 定出 PWM 频率，返回一个周期的计数值
 *   TIM3_EnablePwm(ch, active_high)  通道进 PWM 模式 1 并打开输出
 *   TIM3_SetDuty(ch, percent)        按百分比设占空比，0 恒无效、>=100 恒有效
 *   TIM3_DisablePwm(ch)              关通道输出（把引脚交还 GPIO 前必须调）
 *   TIM3_Start() / TIM3_Stop()       计数器 CEN 开关
 *
 * 内部辅助（上层一般不用直接调）：
 *   TIM3_Ccmr(ch) / TIM3_Ccr(ch)              按通道取寄存器地址
 *   TIM3_CcmrBits(ch, ...) / TIM3_CcerBits()  按通道取寄存器里的位
 *
 * 一次完整的 PWM 输出，寄存器上的顺序是：
 *   1. RCC->APB1ENR 置 TIM3EN          —— 不开时钟，后面所有读写都不生效
 *   2. （可选）RCC->APB1RSTR 复位一次  —— 让 TIM3 从复位值起配，不留上一次的残留
 *   3. TIM3_SetFreq(hz)                —— 写 PSC/ARR，定出波形的“时间刻度”
 *   4. TIM3_EnablePwm(ch, 极性)         —— 写 CCMRn/CCER，把通道变成 PWM 输出
 *   5. TIM3_SetDuty(ch, percent)        —— 写 CCRn，定出高电平占多少
 *   6. TIM3_Start()                     —— CR1.CEN=1，计数器跑起来，波形才真正出现
 *   收口时反过来：先 SetDuty(ch, 0) 再 Stop()，最后 DisablePwm(ch) 才允许改 GPIO。
 *
 * 使用约定：
 *   - 本驱动不涉及 RCC，调用前先使能 RCC->APB1ENR 的 TIM3EN（bit1），
 *     时钟没开时读写 TIM3 寄存器不生效。
 *   - 本驱动不涉及 GPIO，OCx 要真正出现在引脚上，需把对应引脚配成复用推挽
 *     （GPIO_MODE_AF_PP_50M）。中容量 F103 上 TIM3 只有默认映射（TIM3_REMAP=00）
 *     和部分重映射（01）两种：CH1=PA6/PB4、CH2=PA7/PB5、CH3=PB0、CH4=PB1，
 *     完整重映射（CH1~CH4=PC6~PC9）在中容量产品上没有。
 *     本板 PA6/PA7 已被数码管占用，所以实际可用的是 PB0(CH3) 和 PB1(CH4)。
 *   - 不开任何中断：DIER 完全不碰，CCxIE/UIE 保持复位值 0，只做输出。
 *   - TIM3 是 16 位定时器，ARR/PSC 各 16 位；不分频时频率下限约 clk/65536。
 *   - TIM3 属于基本定时器，没有 BDTR，也就没有 MOE/死区那一套（只有高级定时器
 *     TIM1/TIM8 才有），不存在“忘了开主输出所以没波形”这个坑。
 *   - ARPE/OCxPE 由本驱动打开，运行中改频率或占空比都在下一个更新事件生效，
 *     不会输出半周期毛刺。
 *   - EnablePwm/DisablePwm/Start/Stop/SetDuty 对寄存器是读改写，整体非原子：
 *     同一个寄存器里还放着别的通道（或别的控制位），勿与中断并发操作。
 *     只有 CCRn 的单次写入本身是原子的。
 *   - static inline 没有函数符号，调试时直接看 TIM3->PSC/ARR/CCR1/CCER/CR1。
 */

#ifndef __DRIVERS_TIM3_H
#define __DRIVERS_TIM3_H

#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief 比较通道编号（值就是硬件手册里的通道序号，1~4）
 * @note  括号内是默认映射（TIM3_REMAP=00）下的引脚，部分重映射（01）见文件头。
 *        本驱动不校验非法值，强转进来的越界通道号会被当成 CH4 处理。
 */
typedef enum
{
    TIM3_CH1 = 1,   /* PA6，本板被数码管 g 段占用 */
    TIM3_CH2 = 2,   /* PA7，本板被数码管 dp 段占用 */
    TIM3_CH3 = 3,   /* PB0，本板接蜂鸣器 */
    TIM3_CH4 = 4    /* PB1 */
} TIM3_Channel;

/*
 * TIM3 只有 4 个通道，位域在每个寄存器里的位置是固定的，所以这里不做
 * “按通道号算移位量”的宏（那种写法要读者现场心算 (ch-1)&1 之类的式子）。
 * 改成两个查表函数，把每个通道对应的位直接按参考手册的位名列出来，
 * 见下面的 TIM3_CcmrBits() / TIM3_CcerBits()。
 *
 * 需要记住的两点布局：
 *   CCMR1 = CH1(低 8 位) + CH2(高 8 位)，CCMR2 = CH3(低 8 位) + CH4(高 8 位)
 *   CCER  = 每个通道占 4 位，依次是 CCxE、CCxP（TIM3 没有互补输出，
 *           CCxNE/CCxNP 那两位用不上）
 *
 * TIM3 输出 PWM 时真正会碰到的寄存器只有这几个（RM0008 的 14.4 节）：
 *   CR1   bit0 CEN    计数器使能，PWM 波形的前提开关
 *         bit7 ARPE   ARR 是否走预装载；本驱动打开它，改频率才不会有毛刺
 *   PSC   16 位       输入分频，决定 CNT 走一步有多久
 *   ARR   16 位       重装载值，CNT 从 0 数到 ARR 就翻转，所以周期是 ARR+1 个计数
 *   CCMR1 CCMR2       每通道 8 位：CCxS(输入选择) OCxFE(快速) OCxPE(预装载) OCxM(模式)
 *   CCR1~CCR4         比较值，决定一个周期里有效电平占多长
 *   CCER  每通道 4 位：CCxE(输出使能) CCxP(极性)
 *   EGR   bit0 UG     手动产生一次更新事件，把影子寄存器立刻灌进工作寄存器
 *   SR    bit0 UIF    更新事件标志，本驱动不开中断，纯手动清零
 *   DIER              本驱动完全不碰，保持复位值 0
 */

/**
 * @brief TIM3 的计数时钟（Hz）
 * @return 该频率 /((PSC+1)×(ARR+1)) 就是 PWM 的实际频率
 * @note  取值链：SYSCLK(SystemCoreClock) -> /HPRE = HCLK -> /PPRE1 = PCLK1。
 *        RM0008 规定 APB1 预分频不为 1 时，挂在 APB1 上的定时器时钟是 PCLK1 的两倍，
 *        所以本工程（AHB=1、APB1=/2）算出来是 72MHz，不是 36MHz。
 * @note  不硬编码频率：换主频或改分频后这里自动跟着变，上层只管传 Hz。
 * @note  只读不写，可以在任何上下文里调；内部是纯整数运算，没有除法以外的调用。
 */
static inline uint32_t TIM3_ClockHz(void)
{
    /* CFGR 里关心的两段：
     *   bit 7:4  HPRE   AHB 分频，SYSCLK -> HCLK
     *   bit 10:8 PPRE1  APB1 分频，HCLK   -> PCLK1（TIM3 就挂在 APB1 上）
     * RCC_CFGR_HPRE / RCC_CFGR_PPRE1 是厂商头文件里的掩码，取完还要移位。 */
    uint32_t cfgr  = RCC->CFGR;
    uint32_t hclk  = SystemCoreClock;            /* 此刻等价于 SYSCLK，见下面的逐步除法 */
    uint32_t hpre  = (cfgr & RCC_CFGR_HPRE) >> 4u;
    uint32_t ppre1 = (cfgr & RCC_CFGR_PPRE1) >> 8u;
    uint32_t div1;

    /* HPRE 是 4 位字段：0xxx 一律不分频（包含 0000~0111 这 8 个编码，硬件只用 0000），
     * 1000~1011 是 2/4/8/16 分频，1100~1111 是 64/128/256/512 分频。
     * 移位量正好等于"编码减 7"，所以两个区间合并成一条移位；
     * 1000 -> >>1 -> /2，1111 -> >>8 -> /512，逐个对得上。 */
    if (hpre >= 8u)
    {
        hclk >>= (hpre >= 12u) ? (hpre - 6u) : (hpre - 7u);
    }

    /* PPRE1 是 3 位字段：0xx 不分频，100/101/110/111 依次是 2/4/8/16，
     * 101 之后 1101~1111 是保留值。本工程配的是 /2（编码 100），div1 = 2。 */
    div1 = (ppre1 < 4u) ? 1u : (1u << (ppre1 - 3u));

    /* 最后一步是 RM0008 时钟树的规则，不是笔误：
     *   PPRE1 = 1   -> TIM3 时钟 = PCLK1
     *   PPRE1 != 1  -> TIM3 时钟 = PCLK1 × 2
     * 所以 AHB=1、APB1=/2 的 72MHz 系统里，HCLK=72M、PCLK1=36M，
     * 而 TIM3 拿到的是 72MHz。APB2 上的 TIM1 则永远等于 PCLK2，不吃这个 ×2。 */
    return (div1 == 1u) ? hclk : (hclk / div1 * 2u);
}

/** @brief 按通道号取比较模式寄存器：CH1/CH2 在 CCMR1，CH3/CH4 在 CCMR2 */
static inline __IO uint16_t *TIM3_Ccmr(TIM3_Channel ch)
{
    return (ch <= TIM3_CH2) ? &TIM3->CCMR1 : &TIM3->CCMR2;
}

/** @brief 按通道号取 CCR。结构体里每个 CCR 后面都跟着一个 16 位 RESERVED，
 *         不能靠数组下标取，只能逐通道分支。越界通道号落到 CH4。 */
static inline __IO uint16_t *TIM3_Ccr(TIM3_Channel ch)
{
    switch (ch)
    {
        case TIM3_CH1: return &TIM3->CCR1;
        case TIM3_CH2: return &TIM3->CCR2;
        case TIM3_CH3: return &TIM3->CCR3;
        default:       return &TIM3->CCR4;
    }
}

/**
 * @brief 取该通道在 CCMR 里要改的位（把通道号翻译成位，不做移位计算）
 * @param ch     通道号
 * @param clear  输出：要清成 0 的位 —— 输入选择 CCxS、快速使能 OCxFE、
 *               预装载 OCxPE、模式 OCxM，即手册里这个通道低 7 位那一段
 * @param set    输出：要置 1 的位 —— OCxM=110（PWM 模式 1）加 OCxPE=1
 * @note  CCxS（输入选择）厂商头文件里没给宏，而做 PWM 输出必须它是 00，
 *        所以这里按手册的位置写成裸值：CH1/CH3 是 0x0003（bit1:0），
 *        CH2/CH4 是 0x0300（bit9:8）。
 * @note  set 里带上 OCxPE 是有意重复 clear 过的位：先清后置，逻辑上不用记
 *        上一次残留了什么。
 * @note  四个位段各自的含义（以 CH1 为例，其余同构）：
 *        CC1S  00=输出比较。留 1x 会变成输入捕获/触发/从模式，那时 OC1RE 方向反转。
 *        OC1FE 0 关掉快速模式：快速模式会把预装载 bypass 掉，做 PWM 不需要。
 *        OC1PE 1 让 CCR1 走预装载，改占空比在下一个更新事件才生效。
 *        OC1M  110=PWM 模式 1：OC1REF 在 CNT < CCR1 时为有效电平，否则为无效电平。
 */
static inline void TIM3_CcmrBits(TIM3_Channel ch, uint16_t *clear, uint16_t *set)
{
    switch (ch)
    {
        case TIM3_CH1:   /* CCMR1 低字节 */
            *clear = (uint16_t)(0x0003u | TIM_CCMR1_OC1FE | TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M);
            *set   = (uint16_t)(TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1);
            break;

        case TIM3_CH2:   /* CCMR1 高字节 */
            *clear = (uint16_t)(0x0300u | TIM_CCMR1_OC2FE | TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M);
            *set   = (uint16_t)(TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1);
            break;

        case TIM3_CH3:   /* CCMR2 低字节，本板蜂鸣器用的就是这一路 */
            *clear = (uint16_t)(0x0003u | TIM_CCMR2_OC3FE | TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M);
            *set   = (uint16_t)(TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1);
            break;

        default:         /* CCMR2 高字节；越界通道号也落到这里，与 TIM3_Ccr() 保持一致 */
            *clear = (uint16_t)(0x0300u | TIM_CCMR2_OC4FE | TIM_CCMR2_OC4PE | TIM_CCMR2_OC4M);
            *set   = (uint16_t)(TIM_CCMR2_OC4PE | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1);
            break;
    }
}

/** @brief 取该通道在 CCER 里的两个位：CCxE 输出使能、CCxP 输出极性（1=反相）
 * @note  CCxE=0 时输出级直接关闭，引脚交还给 GPIO 自己的配置（配成输入就是浮空/上下拉），
 *         这也是“想把引脚还给普通 GPIO，先调 TIM3_DisablePwm()”的原因。
 *         CCxP 只影响极性，不关闭任何东西；两个位一起清是顺手复位，避免残留反相。 */
static inline void TIM3_CcerBits(TIM3_Channel ch, uint16_t *enable, uint16_t *polarity)
{
    switch (ch)
    {
        case TIM3_CH1: *enable = TIM_CCER_CC1E; *polarity = TIM_CCER_CC1P; break;
        case TIM3_CH2: *enable = TIM_CCER_CC2E; *polarity = TIM_CCER_CC2P; break;
        case TIM3_CH3: *enable = TIM_CCER_CC3E; *polarity = TIM_CCER_CC3P; break;
        default:       *enable = TIM_CCER_CC4E; *polarity = TIM_CCER_CC4P; break;  /* 越界落到 CH4 */
    }
}

/**
 * @brief 设定 PWM 频率：自动挑一组 (PSC, ARR) 让重装载值塞进 16 位
 * @param  freq_hz  期望的 PWM 频率
 * @return 一个周期的计数值（ARR+1，占空比按它换算）；参数非法返回 0，不写寄存器
 * @note  整数除法向下取整，实际频率 = TIM3_ClockHz() / ((PSC+1) * 返回值)，
 *        比期望值略高一点（本板 72MHz / 2000Hz = 36000 整除，没有偏差）。
 * @note  计数器停着也可以配频率，波形等 TIM3_Start() 之后才出现；
 *        反过来，运行中改频率也没问题，末尾的 UG 会让新的 PSC/ARR 立刻生效。
 * @note  末尾写一次 EGR.UG，把刚设的 PSC/ARR 立刻装进影子寄存器，
 *        不这么做的话要等计数器数完一轮才生效；UG 会顺带置 SR.UIF，随后清掉，
 *        免得以后使能更新中断时凭空多进一次。
 */
static inline uint32_t TIM3_SetFreq(uint32_t freq_hz)
{
    uint32_t clk    = TIM3_ClockHz();
    uint32_t counts = 0u;
    uint32_t psc;

    /* 频率为 0 没有意义；高于计数时钟则一个周期连一个计数都不够。
     * 这两种情况直接返回 0，让调用方自己判断，不写寄存器、不破坏当前输出。 */
    if ((freq_hz == 0u) || (freq_hz > clk))
    {
        return 0u;
    }

    /* 两层关系：
     *     计数频率 f_cnt = clk / (PSC + 1)
     *     PWM 周期       = ARR + 1 个 f_cnt
     * => PWM 频率        = clk / ((PSC + 1) * (ARR + 1))
     * ARR 只有 16 位，counts 必须落在 1~65536；先从最小分频试起，
     * 分频越小占空比分辨率越高，所以“第一个能用”的那组就是最优解。
     * counts 是向上取整前的整数商，宁大不小，因此实际频率总是略高于期望值。 */
    for (psc = 0u; psc <= 0xFFFFu; psc++)
    {
        counts = clk / (psc + 1u) / freq_hz;
        if ((counts >= 1u) && (counts <= 0x10000u))
        {
            break;
        }
    }

    if (psc > 0xFFFFu)    /* 分频拉满也凑不出合法周期，说明频率过低 */
    {
        return 0u;
    }

    TIM3->PSC = (uint16_t)psc;                  /* 低 16 位有效，高 16 位保留必须写 0 */
    TIM3->ARR = (uint16_t)(counts - 1u);        /* 存的是“最后一个计数值”，比周期小 1 */

    /* ARPE=1：ARR 走预装载。没有它，counts 变小时 CNT 可能一时超过新 ARR，
     * 触发一次非预期的回绕（输出多出半个周期）。
     * 注意 PSC 不受 ARPE 管——PSC 的影子寄存器始终存在，但要更新事件才灌进去。 */
    TIM3->CR1 |= TIM_CR1_ARPE;

    /* 手动产生一次更新事件（UG 写 1 后由硬件自动清零）：
     * 把上面刚写的 PSC/ARR 立刻送进工作寄存器，并让 CNT 从 0 重新数，
     * 顺带也会把 CCRx 的预装载值灌进影子——此刻 CCR 还没配，是无害的。
     * UG 同时置起 SR.UIF。 */
    TIM3->EGR = TIM_EGR_UG;

    /* 清掉 UIF（写 0 无效、写 1 清零的规则只适用于“硬件置位的标志位”，
     * 这里整体写 0 是最省事的清法）。DIER 没开，其实不会产生中断，
     * 清零只是不给后面使能更新中断的人留一个凭空的第二次进入。 */
    TIM3->SR  = 0u;

    return counts;
}

/**
 * @brief 把通道配成 PWM 输出并打开输出使能
 * @param ch           TIM3_CH1 ~ TIM3_CH4
 * @param active_high  非 0：有效期间输出高（CCxP=0）；0：反相，有效期间输出低
 * @note  PWM 模式 1 的语义是 CNT < CCR 时输出有效电平，所以占空比 = CCR/(ARR+1)；
 *        CCxS 同时清成 00（输出模式），OCxPE 置 1（比较值走影子寄存器）。
 * @note  调用顺序上必须在 TIM3_SetFreq() 之后：它只配通道，不知道也不改 PSC/ARR，
 *        频率还没定时先开通道，输出的是一轮复位值下的波形。
 * @note  非原子读改写，理由见文件头。
 */
static inline void TIM3_EnablePwm(TIM3_Channel ch, uint32_t active_high)
{
    __IO uint16_t *ccmr = TIM3_Ccmr(ch);
    uint16_t clear;                 /* CCMR：本通道要清成 0 的位 */
    uint16_t set;                   /* CCMR：本通道要置 1 的位 */
    uint16_t ccxe;                  /* CCER：本通道的输出使能位 */
    uint16_t ccxp;                  /* CCER：本通道的输出极性位 */
    uint16_t ccer;

    TIM3_CcmrBits(ch, &clear, &set);
    TIM3_CcerBits(ch, &ccxe, &ccxp);

    /* 先把这个通道在 CCMR 里的位全清掉，再写成 PWM 模式 1 + 比较值预装载。
     * 必须读改写而不是整寄存器赋值：同一个 CCMR 里还躺着另一个通道，
     * 直接赋值会把那边的配置抹掉。 */
    *ccmr = (uint16_t)((*ccmr & (uint16_t)~clear) | set);

    /* CCxE=1 打开输出；CCxP 决定有效期间是高还是低。
     * 本工程蜂鸣器是低边开关（PB0 高 -> SS8050 导通 -> 蜂鸣器通电），
     * 传 active_high=1，让“有效电平”就是高电平，占空比才等于通电时间比。
     * 反过来接（PNP 高边驱动）就要传 0，逻辑不用改。 */
    ccer = (uint16_t)(TIM3->CCER & (uint16_t)~(ccxe | ccxp));
    ccer = (uint16_t)(ccer | ccxe);
    if (!active_high)
    {
        ccer = (uint16_t)(ccer | ccxp);
    }
    TIM3->CCER = ccer;
}

/**
 * @brief 按百分比设占空比（整数 0~100）
 * @param ch       TIM3_CH1 ~ TIM3_CH4
 * @param percent  0 => CCR=0，输出恒为无效电平；>=100 => CCR=ARR+1，恒为有效电平；
 *                 中间值按 CCR = (ARR+1) * percent / 100 取整。
 * @note  占空比分辨率取决于当前 (ARR+1)：2kHz 时是 36000 级，用百分比接口等于
 *        把它压成 101 级，够蜂鸣器用；要更细就自己直接写 TIM3_Ccr(ch)。
 * @note  ARR=0xFFFF 时 CCR=ARR+1 超出 16 位写不进去，只能取 0xFFFF，
 *        也就是"100%"实际差一个计数。
 * @note  这里读到的 ARR 是工作值（ARPE=1 时即上一次更新事件装进去的那个），
 *        刚调用完 TIM3_SetFreq() 不用担心——它内部已经用 UG 强制装载过一次。
 *        写进 CCRx 的值受 OCxPE 管辖，下一个更新事件才反映到输出上。
 *        上层若只想要“一条原子写”，可以直接给 *TIM3_Ccr(ch) 赋值（bsp_beep 就这么做）。
 */
static inline void TIM3_SetDuty(TIM3_Channel ch, uint32_t percent)
{
    uint32_t period = (uint32_t)TIM3->ARR + 1u;   /* CNT 数 0..ARR，共 ARR+1 个状态 */
    uint32_t ccr    = (period * percent) / 100u;

    if (ccr > period)      /* percent > 100 时钳到 period：CCR >= ARR+1 即 CNT 恒小于它 */
    {
        ccr = period;
    }
    if (ccr > 0xFFFFu)     /* 只有 period = 65536（ARR = 0xFFFF）这一档会触发 */
    {
        ccr = 0xFFFFu;
    }

    *TIM3_Ccr(ch) = (uint16_t)ccr;
}

/** @brief 关通道输出（CCxE=0）。想把该引脚交还给普通 GPIO，先调这个再改 GPIO 模式。
 *         顺手把 CCxP 也清掉，下次 EnablePwm 会按参数重新设。
 * @note  只关输出级，不改 CCR：占空比、频率配置都留着，
 *         再调一次 TIM3_EnablePwm() 会立刻按原来的波形恢复输出。
 * @note  CCxE=0 之后引脚归 GPIO 自己管，所以调用方如果是把引脚切回普通输出，
 *         要在切模式前先把 ODR 写好，否则会看到一次电平跳变。 */
static inline void TIM3_DisablePwm(TIM3_Channel ch)
{
    uint16_t ccxe;                  /* CCER：本通道的输出使能位 */
    uint16_t ccxp;                  /* CCER：本通道的输出极性位 */

    TIM3_CcerBits(ch, &ccxe, &ccxp);
    TIM3->CCER = (uint16_t)(TIM3->CCER & (uint16_t)~(ccxe | ccxp));
}

/** @brief 启动计数器（CR1.CEN=1），PWM 波形从这一刻开始出现。
 * @note  RM0008 写明：软件把 CEN 置 1 之后，计数器要到下一个 CK_CNT 沿才真正动第一步，
 *         也就是最多延迟一个“分频后的计数周期”（默认 2700Hz 时 PSC=0，
 *         CK_CNT 就是 72MHz，延迟不到 15ns；PSC 配大时按 (PSC+1)/72MHz 换算）。
 *         调用方不需要为此等待。
 * @note  置 1 的同时硬件会把 UIF 清 0，所以这里不用管 SR。 */
static inline void TIM3_Start(void)
{
    TIM3->CR1 |= TIM_CR1_CEN;
}

/** @brief 停计数器（CR1.CEN=0）。
 * @note  CEN 清 0 只是让 CNT 停下，**不复位 CNT**，再 Start 是从当前值接着数。
 *         做 PWM 无所谓（波形仍由 CNT 与 CCR 的比较决定），
 *         要是拿同一个定时器做精确周期，得先手动把 CNT 清 0。
 * @note  停止后引脚电平停在最后一次比较结果上：若停的瞬间 CNT < CCR，输出会锁在
 *         有效电平上。蜂鸣器这类负载要先 TIM3_SetDuty(ch, 0) 再停，否则会一直响。
 *         这也是 bsp_beep 用“占空比归 0”而不是“停定时器”来做静音的原因——
 *         计数器一直跑着，随时改 CCR 就能立刻换音高，不用重新对齐相位。 */
static inline void TIM3_Stop(void)
{
    TIM3->CR1 &= (uint16_t)~TIM_CR1_CEN;
}

#endif /* __DRIVERS_TIM3_H */
