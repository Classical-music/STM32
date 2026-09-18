#include "rtc/bsp_rtc.h"
#include <time.h>

/* 模拟器用 PC 主机时钟模拟 bsp 层 RTC 秒计数器。
 * task_rtc.c 与目标板使用同一份源码, 只有这一层是模拟实现,
 * 因此模拟器上的换算/校时行为与目标板一致(时间跟随 PC 时钟走时)。
 */
static time_t g_t0; /* 计数器为 0 对应的主机时间 */

void bsp_rtc_init(void) { g_t0 = time(NULL); }

bool bsp_rtc_is_ready(void) { return true; }

uint32_t bsp_rtc_get_counter(void) { return (uint32_t)(time(NULL) - g_t0); }

void bsp_rtc_set_counter(uint32_t counter) { g_t0 = time(NULL) - (time_t)counter; }
