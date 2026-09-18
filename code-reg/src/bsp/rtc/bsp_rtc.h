#ifndef __BSP_RTC_H__
#define __BSP_RTC_H__

#include "type.h"

// 初始化 RTC: 配置 LSE(32768Hz) 作为时钟源, 预分频到 1Hz 秒计数器
// 通过备份域魔数判断是否已配置, 复位后不清零、继续走时
void bsp_rtc_init(void);

// RTC 是否已成功配置并处于走时状态
bool bsp_rtc_is_ready(void);

// 读取 RTC 秒计数器 (自上次设置以来经过的秒数, 32 位)
// RTC 未就绪时返回 0
uint32_t bsp_rtc_get_counter(void);

// 设置 RTC 秒计数器
// RTC 未就绪时忽略
void bsp_rtc_set_counter(uint32_t counter);

#endif
