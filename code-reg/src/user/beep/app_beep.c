#include "app_beep.h"
#include "timer.h"

static timer_id_t beep_timer_id = -1;
static uint32_t delay = 1000;

static void beep_toggle_timer_cb(void *arg) {
  beep_toggle(); // 反转 beep
}

void app_beep_play(void) {
  if (beep_timer_id == -1) {
    // 未启动, 先响, 再启动定时器
    beep_on();
    beep_timer_id = TIMER_ADD_CB(delay, beep_toggle_timer_cb, NULL, true);
  } else {
    // 已启动, 先结束定时器, 再灭
    timer_del(beep_timer_id);
    beep_timer_id = -1;
    app_beep_stop();
    beep_off();
  }
}

void app_beep_stop(void) {
  if (beep_timer_id != -1) {
    timer_del(beep_timer_id);
    beep_timer_id = -1;
  }
}

void app_beep_del(uint32_t dec_delay) {
  if (beep_timer_id == -1)
    return; // 未启动直接返回
  delay -= dec_delay;
  if (delay < 50)
    delay = 1000; // 延时最少到 50
  // 先结束定时器
  timer_del(beep_timer_id);
  // 再重启定时器
  beep_timer_id = TIMER_ADD_CB(delay, beep_toggle_timer_cb, NULL, true);
}

void playAlert(void) {
  // 先结束现有定时器
  app_beep_stop();

  beep_on(); // 开启beep

  // 定时单次, 20ms后关闭
  TIMER_ADD_CB(20, beep_toggle_timer_cb, NULL, false);
}
