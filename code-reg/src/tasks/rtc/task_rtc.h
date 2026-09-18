#ifndef __TASK_RTC_H__
#define __TASK_RTC_H__

#include "type.h"

/* RTC 时间结构, 表示本地时间(东八区 UTC+8)。 */
typedef struct {
  uint16_t year;   /* 四位数年份, 如 2026 */
  uint8_t month;   /* 月, 1~12 */
  uint8_t day;     /* 日, 1~31 */
  uint8_t hour;    /* 时, 0~23 */
  uint8_t minute;  /* 分, 0~59 */
  uint8_t second;  /* 秒, 0~59 */
} rtc_time_t;

/* 本地时区偏移: 东八区, 秒。显示时间 = epoch + 该偏移。 */
#define RTC_UTC_OFFSET_SECONDS (8u * 3600u)

/* 初始化 RTC 服务: 以固件编译日期时间作为基础时间并清零计数器。
 * 需在 board_init()(内部已调用 bsp_rtc_init) 之后、任务启动前调用。
 */
void rtcTaskInit(void);

/* RTC 硬件(备份域)是否已成功配置并走时。 */
bool rtcIsReady(void);

/* 获取当前 Unix epoch(自 1970-01-01 00:00:00 UTC 起的秒数)。
 * = 基础 epoch + bsp 秒计数器; RTC 未就绪时返回基础 epoch。
 */
uint32_t rtcGetEpoch(void);

/* 设置当前时间基准: 记录基础 epoch 并把秒计数器清零。 */
void rtcSetEpoch(uint32_t epoch);

/* epoch -> 本地时间(UTC+8)。out 不允许为 NULL。 */
void rtcEpochToLocal(uint32_t epoch, rtc_time_t *out);

/* 本地时间(UTC+8) -> epoch。
 * 字段非法(越界/无效日期/超出可表示范围)时返回 false。
 */
bool rtcLocalToEpoch(const rtc_time_t *in, uint32_t *out);

/* 获取当前本地时间。 */
void rtcGetLocalTime(rtc_time_t *out);

/* 用本地时间校时。字段非法或 RTC 未就绪时返回 false。 */
bool rtcSetLocalTime(const rtc_time_t *in);

/* 将当前本地时间格式化为 "YYYY-MM-DD HH:MM:SS", 写入 buf 并返回 buf。
 * buf_size 建议不小于 20。
 */
char *rtcFormatNow(char *buf, uint32_t buf_size);

#endif
