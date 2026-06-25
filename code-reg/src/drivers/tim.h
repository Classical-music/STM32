#ifndef __TIM_H__
#define __TIM_H__

#include "type.h"

// 初始化 pwm 输出模式 TIM3
void tim_pwm_init(void);
void tim_pwm_start(void);
void tim_pwm_stop(void);
// 设置 pwm 占空比
void tim_pwm_dutyCycle(uint8_t pct);

// TIM5-CH2: PA1 输入捕获功能
void tim_cap_init(void);
void tim_cap_start(void);
void tim_cap_stop(void);
uint16_t tim_cap_get_val(void);
void tim_cap_reset(void);

#endif
