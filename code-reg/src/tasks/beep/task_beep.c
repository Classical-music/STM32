#include "task_beep.h"
#include "beep/beep.h"
#include "timers.h"

/* 蜂鸣器软件定时器及当前播放状态。 */
static TimerHandle_t xBeepTimer = NULL;
/* true 表示当前使用播放/停止交替的循环模式。 */
static bool beep_loop_mode;
/* 记录当前蜂鸣器输出是否处于开启状态。 */
static bool beep_output_on;
/* 循环模式下的播放时长和停止时长。 */
static uint32_t beep_play_ms;
static uint32_t beep_stop_ms;

/* 定时器到期后关闭蜂鸣器，并在循环模式下重新开始播放。 */
static void vBeepTimerHandler(TimerHandle_t xTimer) {
  (void)xTimer;

  if (!beep_loop_mode) {
    beep_off();
    beep_output_on = false;
    return;
  }

  if (beep_output_on) {
    beep_off();
    beep_output_on = false;
    xTimerChangePeriod(xBeepTimer, pdMS_TO_TICKS(beep_stop_ms), 0);
  } else {
    beep_on();
    beep_output_on = true;
    xTimerChangePeriod(xBeepTimer, pdMS_TO_TICKS(beep_play_ms), 0);
  }
}

void beepTaskInit(void) {
  if (xBeepTimer != NULL)
    return;

  xBeepTimer =
      xTimerCreate("beepTimer", pdMS_TO_TICKS(100), pdFALSE, NULL,
                   vBeepTimerHandler);
}

void playBeep(uint32_t duration_ms) {
  TickType_t duration_ticks;

  if (xBeepTimer == NULL || duration_ms == 0)
    return;

  beep_loop_mode = false;
  duration_ticks = pdMS_TO_TICKS(duration_ms);
  if (duration_ticks == 0)
    duration_ticks = 1;

  beep_on();
  beep_output_on = true;
  if (xTimerChangePeriod(xBeepTimer, duration_ticks, pdMS_TO_TICKS(100)) !=
      pdPASS) {
    beep_output_on = false;
    beep_off();
  }
}

/* 开始循环播放。
 * 进入函数时立即进入“播放”阶段，经过 play_ms 后关闭；
 * 再经过 stop_ms 后重新打开，如此交替执行。
 */
void playBeepLoop(uint32_t play_ms, uint32_t stop_ms) {
  TickType_t play_ticks;

  if (xBeepTimer == NULL || play_ms == 0 || stop_ms == 0)
    return;

  beep_loop_mode = true;
  beep_play_ms = play_ms;
  beep_stop_ms = stop_ms;
  play_ticks = pdMS_TO_TICKS(play_ms);
  if (play_ticks == 0)
    play_ticks = 1;

  beep_on();
  beep_output_on = true;
  if (xTimerChangePeriod(xBeepTimer, play_ticks, pdMS_TO_TICKS(100)) !=
      pdPASS) {
    beep_loop_mode = false;
    beep_output_on = false;
    beep_off();
  }
}

/* 立即停止当前播放。
 * 先停止软件定时器，避免已经排队的定时器命令再次改变蜂鸣器状态；
 * 然后清除循环播放状态并关闭 GPIO 输出。
 */
void stopBeep(void) {
  if (xBeepTimer != NULL)
    xTimerStop(xBeepTimer, pdMS_TO_TICKS(100));

  beep_loop_mode = false;
  beep_output_on = false;
  beep_off();
}
