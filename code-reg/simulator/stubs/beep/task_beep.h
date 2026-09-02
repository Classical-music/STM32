#ifndef TASK_BEEP_H
#define TASK_BEEP_H

#include <stdint.h>

/* 模拟器不连接真实蜂鸣器。
 * 这里提供与 STM32 端相同的接口，使共享页面代码无需区分平台。
 */
static inline void beepTaskInit(void) {}

/* 模拟单次蜂鸣器播放。
 * 模拟器当前不输出声音，因此只接收并忽略播放时长参数。
 */
static inline void playBeep(uint32_t duration_ms) {
  (void)duration_ms;
}

/* 模拟循环蜂鸣器播放。
 * 模拟器当前不输出声音，因此只接收并忽略两个时序参数。
 */
static inline void playBeepLoop(uint32_t play_ms, uint32_t stop_ms) {
  (void)play_ms;
  (void)stop_ms;
}

/* 模拟器中立即停止蜂鸣器播放。 */
static inline void stopBeep(void) {}

/* 与 STM32 端保持一致的默认短提示音接口。 */
#define playAlert() playBeep(20)

#endif
