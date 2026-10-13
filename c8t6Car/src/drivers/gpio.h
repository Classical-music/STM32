/**
 * @file  gpio.h
 * @brief 通用 GPIO 驱动（纯宏实现，直接操作 CRL/CRH/IDR/BSRR/BRR，无 .c 文件，
 *        不依赖标准外设库）
 *
 * 对外接口四个，port 传 GPIOA~GPIOG，pin 一律传 0~15 的编译期常量：
 *   GPIO_SET_MODE(port, mode, 13, 14, 15)   批量配置引脚模式，末参数是引脚编号列表
 *   GPIO_HIGH(port, pin)                    输出高
 *   GPIO_LOW(port, pin)                     输出低
 *   GPIO_READ(port, pin)                    读输入电平，返回 0/1
 *
 * 使用约定：
 *   - 本驱动不涉及 RCC，端口时钟由上层自行使能：RCC->APB2ENR 的 bit2~bit8
 *     依次对应 GPIOA~GPIOG，时钟没开时读写寄存器不生效。
 *   - 宏内不做引脚编号校验，越界不是"无操作"而是"改到别的引脚"：HIGH/LOW 的
 *     1u << pin 会落进 BSRR/BRR 的高 16 位（写 16 等于复位 pin0），READ 的移位量
 *     在 ARM 上按 32 取模（pin 32 等于读 pin0）。调用方自己保证 0~15。
 *   - GPIO_SET_MODE 的引脚列表展开成函数内 static const uint8_t 数组（放 Flash），
 *     元素必须是编译期常量：传变量报 Error #28，一项都不传会展开成空初始化列表而报错。
 *     单个引脚只写一项即可；同一引脚重复出现时后一项生效。
 *   - 宏会对实参多次求值（port 在 CRL/CRH 两个分支里各展开一次，mode 每次写寄存器
 *     都用一次），实参只写 GPIOA~GPIOG 与 GPIO_MODE_xxx 这类常量，不要带自增、
 *     函数调用等副作用。
 *   - -O1 下 GPIO_SET_MODE 保留运行时循环（读引脚表 + 逐引脚比较分支），不会折叠成
 *     直线代码；模式配置集中在上电初始化阶段做，勿放进高频路径。
 *   - GPIO_SET_MODE 对 CRL/CRH 做 &= 再 |= 两次访问，整体非原子：勿与中断并发操作
 *     同一端口。
 *   - GPIO_HIGH/GPIO_LOW 走 BSRR/BRR，单条写完成，原子，中断里可以安全使用。
 *   - 没有函数符号也就没有断点，调试时直接看 CRL/CRH/ODR/IDR 寄存器。
 */

#ifndef __DRIVERS_GPIO_H
#define __DRIVERS_GPIO_H

#include "stm32f10x.h"

/**
 * @brief 引脚模式：枚举值即 CRL/CRH 中该引脚的 4 位配置（高 2 位 CNF + 低 2 位 MODE）
 * @note  输出速度编码不单调：MODE=01 为 10MHz，MODE=10 为 2MHz，MODE=11 为 50MHz；
 *        输入类模式没有速度维度。RM0008 中 CNF=11/MODE=00 为保留态，故未列出。
 */
typedef enum
{
    /* 输入 */
    GPIO_MODE_AIN         = 0x00,  /* 模拟输入（ADC）              */
    GPIO_MODE_IN_FLOATING = 0x04,  /* 浮空输入                      */
    GPIO_MODE_IN_PULL     = 0x08,  /* 上下拉输入，方向由 ODR 位选择  */

    /* 通用输出 */
    GPIO_MODE_OUT_PP_10M  = 0x01,  /* 推挽输出 10MHz                */
    GPIO_MODE_OUT_PP_2M   = 0x02,  /* 推挽输出 2MHz                 */
    GPIO_MODE_OUT_PP_50M  = 0x03,  /* 推挽输出 50MHz                */
    GPIO_MODE_OUT_OD_10M  = 0x05,  /* 开漏输出 10MHz                */
    GPIO_MODE_OUT_OD_2M   = 0x06,  /* 开漏输出 2MHz                 */
    GPIO_MODE_OUT_OD_50M  = 0x07,  /* 开漏输出 50MHz                */

    /* 复用输出（USART/TIM PWM/I2C 等，需另配 AFIO 重映射） */
    GPIO_MODE_AF_PP_10M   = 0x09,  /* 复用推挽 10MHz                */
    GPIO_MODE_AF_PP_2M    = 0x0A,  /* 复用推挽 2MHz                 */
    GPIO_MODE_AF_PP_50M   = 0x0B,  /* 复用推挽 50MHz                */
    GPIO_MODE_AF_OD_10M   = 0x0D,  /* 复用开漏 10MHz                */
    GPIO_MODE_AF_OD_2M    = 0x0E,  /* 复用开漏 2MHz                 */
    GPIO_MODE_AF_OD_50M   = 0x0F,  /* 复用开漏 50MHz                */
} GPIO_Mode;

/* ------------------------------------------------------------ 常量与辅助 */

#define GPIO_MODE_MASK    0x0Fu      /* 单引脚 4 位配置域（CNF[1:0] + MODE[1:0]）的掩码 */

/* -------------------------------------------------------------- 对外接口 */

/* GPIO_SET_MODE 内部用的数组长度算法；名字是通用形式且随本头文件对外可见，
 * 若与其它库的同名宏同时包含，需保证展开一致，否则是宏重定义。 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

/**
 * @brief 批量配置引脚模式（逐个读改写 CRL/CRH 的 4 位域）
 * @param port  GPIOA~GPIOG，直接展开进宏体，请传端口名
 * @param mode  GPIO_MODE_xxx，同一批引脚共用一个模式
 * @param ...   引脚编号列表，写成 13, 14, 15；个数由 ARRAY_SIZE 自己算，不传 count。
 *              单个引脚就只写一项：GPIO_SET_MODE(GPIOC, GPIO_MODE_OUT_PP_2M, 13)
 * @note 位域位置：引脚号 ×4 得起始位，0~28 落在 CRL（pin0~7），32~60 落在 CRH
 *       （pin8~15，减 32 后同样得到 0~28）。
 * @note 编号请写 0~15 的编译期常量。宏本身不校验：16~63 两个分支都不进（不碰寄存器），
 *       但 >=64 会因 uint8_t 存不下 ×4 的结果而回绕成 0~60，被当成合法引脚写入，
 *       所以宏体内的 pos 类型换成 uint32_t 之前，绝不能传 64 以上的值。
 * @note port 在两个分支里各展开一次、mode 每次写寄存器都用一次，实参不要带副作用。
 */
#define GPIO_SET_MODE(port, mode, ...)                          \
        do {                                                    \
            static const uint8_t _pins[] = { __VA_ARGS__ };     \
            for (uint32_t i = 0; i < ARRAY_SIZE(_pins); i++)    \
            {                                                   \
                uint8_t pos = _pins[i] << 2u;                         \
                if (pos < 32u) {                                 \
                    port->CRL &= ~(GPIO_MODE_MASK << pos);      \
                    port->CRL |= (mode << pos);                 \
                } else if (pos < 64u) {                         \
                    pos -= 32u;                                  \
                    port->CRH &= ~(GPIO_MODE_MASK << pos);      \
                    port->CRH |= (mode << pos);                 \
                }                                               \
            }                                                   \
        } while (0)

/** @brief 置高：写 BSRR 低 16 位对应位，单条写原子；上下拉输入模式下等效于选上拉。
 *         pin 必须是 0~15，不校验范围（>=16 会写进 BSRR 高 16 位而误复位别的引脚）。 */
#define GPIO_HIGH(port, pin)    ((port)->BSRR = 1u << (pin))

/** @brief 置低：写 BRR 对应位，单条写原子；上下拉输入模式下等效于选下拉。
 *         pin 必须是 0~15，不校验范围。 */
#define GPIO_LOW(port, pin)     ((port)->BRR = 1u << (pin))

/** @brief 读单引脚输入电平，结果为 0 或 1。
 *         pin 必须是 0~15，不校验范围（16~31 读到保留位恒为 0，>=32 移位量按 32 取模）。 */
#define GPIO_READ(port, pin)    (((port)->IDR >> (pin)) & 1u)

#endif /* __DRIVERS_GPIO_H */
