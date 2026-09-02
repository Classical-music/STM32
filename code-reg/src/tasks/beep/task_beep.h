#ifndef __TASK_BEEP_H__
#define __TASK_BEEP_H__

#include "type.h"

/* 初始化蜂鸣器播放所需的 FreeRTOS 软件定时器。
 * 必须在调用 playBeep() 或 playBeepLoop() 前完成初始化。
 */
void beepTaskInit(void);

/* 播放一次指定时长的蜂鸣声。
 * duration_ms 的单位为毫秒，播放完成后自动关闭蜂鸣器。
 */
void playBeep(uint32_t duration_ms);

/* 按播放时长和停止时长交替循环播放。
 * play_ms  : 每次蜂鸣器开启的时长，单位为毫秒。
 * stop_ms  : 每次蜂鸣器关闭的时长，单位为毫秒。
 */
void playBeepLoop(uint32_t play_ms, uint32_t stop_ms);

/* 立即停止当前蜂鸣器播放，并取消后续定时动作。 */
void stopBeep(void);

/* 播放默认的 20 ms 短提示音。 */
#define playAlert() playBeep(20)

#endif
