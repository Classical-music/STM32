#include "task_rtc.h"
#include "rtc/bsp_rtc.h"
#include <stdio.h>

/*
 * 时间模型: epoch = 基础 epoch(g_base_epoch) + bsp 秒计数器。
 * bsp 层只提供"原始秒计数", 本层负责:
 *   1. epoch 偏移管理(校时 = 改基础 epoch + 计数器清零)
 *   2. epoch <-> 年月日时分秒的公历换算(手写 civil 算法, 纯函数可重入)
 *   3. 固定东八区显示策略(不用 C 库 localtime, 无 TZ 依赖)
 * epoch 使用 uint32_t, 可表示 1970-01-01 ~ 2106-02-07。
 */

/* 时间基准: 秒计数器为 0 时对应的 epoch。 */
static uint32_t g_base_epoch;

#define RTC_SECS_PER_DAY 86400u

/* --------------------------------------------------------------------------
 * 公历 <-> 天数换算 (Howard Hinnant civil_from_days 算法)
 * 以 1970-01-01 为纪元原点, 适用于 1970 年以后的格里高利历日期。
 * ------------------------------------------------------------------------ */

/* 年月日 -> 距 1970-01-01 的天数(可为负, 但本模块只用于 >= 1970 的日期)。 */
static int32_t rtc_days_from_civil(uint16_t y, uint8_t m, uint8_t d) {
  /* 把 1/2 月视为上一年 13/14 月, 使闰日落在年末, 简化整除公式 */
  uint16_t yy = (uint16_t)(y - (m <= 2 ? 1 : 0));
  int32_t era = (int32_t)(yy / 400);            /* 400 年周期数 */
  uint32_t yoe = yy - (uint16_t)(era * 400);    /* 周期内年份 [0,399] */
  uint32_t mp = (uint32_t)(m + (m > 2 ? -3 : 9)); /* 3 月 = 0 ... 次年 2 月 = 11 */
  uint32_t doy = (153u * mp + 2u) / 5u + d - 1u; /* 周期内天数 [0,365] */
  uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  return era * 146097 + (int32_t)doe - 719468; /* 1970-01-01 对应第 0 天 */
}

/* 距 1970-01-01 的天数 -> 年月日。 */
static void rtc_civil_from_days(int32_t z, uint16_t *y, uint8_t *m,
                                uint8_t *d) {
  z += 719468; /* 转到公元纪年(以 0000-03-01 为第 0 天) */
  int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  uint32_t doe = (uint32_t)(z - era * 146097); /* 周期内天数 [0,146096] */
  uint32_t yoe =
      (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u; /* [0,399] */
  uint32_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u); /* 周期内年天 [0,365] */
  uint32_t mp = (5u * doy + 2u) / 153u;                      /* 3 月起算月 [0,11] */
  uint32_t year = yoe + (uint32_t)(era * 400);
  uint32_t month;

  /* 月份还原: mp=0..9 -> 3..12 月, mp=10..11 -> 1..2 月(年份 +1) */
  if (mp < 10u) {
    month = mp + 3u;
  } else {
    month = mp - 9u;
    year += 1u;
  }
  *y = (uint16_t)year;
  *m = (uint8_t)month;
  *d = (uint8_t)(doy - (153u * mp + 2u) / 5u + 1u);
}

static bool rtc_is_leap(uint16_t y) {
  return (y % 4u == 0u) && ((y % 100u != 0u) || (y % 400u == 0u));
}

static uint8_t rtc_days_in_month(uint16_t y, uint8_t m) {
  static const uint8_t days[12] = {31, 28, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31};
  if (m == 2 && rtc_is_leap(y)) {
    return 29;
  }
  return days[m - 1];
}

/* --------------------------------------------------------------------------
 * 编译期时间: __DATE__ 形如 "Sep 18 2026", __TIME__ 形如 "HH:MM:SS"
 * ------------------------------------------------------------------------ */
static const char *const g_month_names[12] = {"Jan", "Feb", "Mar", "Apr",
                                              "May", "Jun", "Jul", "Aug",
                                              "Sep", "Oct", "Nov", "Dec"};

static uint32_t rtc_compile_epoch(void) {
  uint8_t month = 0;
  for (uint8_t i = 0; i < 12; i++) {
    if (__DATE__[0] == g_month_names[i][0] &&
        __DATE__[1] == g_month_names[i][1] &&
        __DATE__[2] == g_month_names[i][2]) {
      month = (uint8_t)(i + 1);
      break;
    }
  }
  /* __DATE__ 定长格式: 月名 0-2, 日 4-5(可能前导空格), 年 7-10 */
  uint16_t day = (uint16_t)((__DATE__[4] - '0') * 10 + (__DATE__[5] - '0'));
  uint16_t year = (uint16_t)((__DATE__[7] - '0') * 1000 +
                             (__DATE__[8] - '0') * 100 +
                             (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0'));
  uint32_t tod = (uint32_t)(__TIME__[0] - '0') * 36000u +
                 (uint32_t)(__TIME__[1] - '0') * 3600u +
                 (uint32_t)(__TIME__[3] - '0') * 600u +
                 (uint32_t)(__TIME__[4] - '0') * 60u +
                 (uint32_t)(__TIME__[6] - '0') * 10u +
                 (uint32_t)(__TIME__[7] - '0');

  return (uint32_t)rtc_days_from_civil(year, month, day) * RTC_SECS_PER_DAY +
         tod;
}

/* --------------------------------------------------------------------------
 * 对外接口
 * ------------------------------------------------------------------------ */

void rtcTaskInit(void) {
  /* 每次启动以编译时间为基准, 计数器从零重新走时;
   * 真实时间需通过 cmd time 或后续 NTP 校时获得 */
  g_base_epoch = rtc_compile_epoch();
  bsp_rtc_set_counter(0);
}

bool rtcIsReady(void) { return bsp_rtc_is_ready(); }

uint32_t rtcGetEpoch(void) { return g_base_epoch + bsp_rtc_get_counter(); }

void rtcSetEpoch(uint32_t epoch) {
  g_base_epoch = epoch;
  bsp_rtc_set_counter(0);
}

void rtcEpochToLocal(uint32_t epoch, rtc_time_t *out) {
  uint32_t local = epoch + RTC_UTC_OFFSET_SECONDS;
  uint32_t secs = local % RTC_SECS_PER_DAY;
  int32_t days = (int32_t)(local / RTC_SECS_PER_DAY);

  /* 一天内的秒数可能使天数进位, 直接由 civil 算法分解 */
  rtc_civil_from_days(days, &out->year, &out->month, &out->day);
  out->hour = (uint8_t)(secs / 3600u);
  out->minute = (uint8_t)(secs % 3600u / 60u);
  out->second = (uint8_t)(secs % 60u);
}

bool rtcLocalToEpoch(const rtc_time_t *in, uint32_t *out) {
  uint32_t epoch;

  if (in == NULL || out == NULL) {
    return false;
  }
  /* uint32 epoch 可表示范围: 1970-01-01 ~ 2106-02-07(UTC) */
  if (in->year < 1970u || in->year > 2106u) {
    return false;
  }
  if (in->month < 1u || in->month > 12u) {
    return false;
  }
  if (in->day < 1u || in->day > rtc_days_in_month(in->year, in->month)) {
    return false;
  }
  if (in->hour > 23u || in->minute > 59u || in->second > 59u) {
    return false;
  }

  int32_t days = rtc_days_from_civil(in->year, in->month, in->day);
  uint32_t tod = (uint32_t)in->hour * 3600u + (uint32_t)in->minute * 60u +
                 in->second;
  if (days < 0) {
    return false;
  }
  epoch = (uint32_t)days * RTC_SECS_PER_DAY + tod;

  /* 本地时间需减去时区偏移得到 UTC epoch, 早于 1970-01-01 08:00 则越界 */
  if (epoch < RTC_UTC_OFFSET_SECONDS) {
    return false;
  }
  *out = epoch - RTC_UTC_OFFSET_SECONDS;
  return true;
}

void rtcGetLocalTime(rtc_time_t *out) {
  if (out == NULL) {
    return;
  }
  rtcEpochToLocal(rtcGetEpoch(), out);
}

bool rtcSetLocalTime(const rtc_time_t *in) {
  uint32_t epoch;

  if (!bsp_rtc_is_ready()) {
    return false;
  }
  if (!rtcLocalToEpoch(in, &epoch)) {
    return false;
  }
  rtcSetEpoch(epoch);
  return true;
}

char *rtcFormatNow(char *buf, uint32_t buf_size) {
  rtc_time_t t;
  rtcGetLocalTime(&t);
  snprintf(buf, buf_size, "%04u-%02u-%02u %02u:%02u:%02u", t.year, t.month,
           t.day, t.hour, t.minute, t.second);
  return buf;
}
