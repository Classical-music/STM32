#include "timer.h"
#include "queue.h"

#define TIMER_MAX 8
static timer_t g_timers[TIMER_MAX];
static uint32_t g_heartbeat_count = 0; // 系统心跳, 精度1ms
static uint32_t g_tick_flag = 0;

void timer_init(void) {
  for (int i = 0; i < TIMER_MAX; i++) {
    g_timers[i].active = false;
  }
  g_heartbeat_count = 0;
  g_tick_flag = 0;
}

timer_id_t timer_add(uint32_t delay_ms, bool cycle, event_id_t evt_type) {
  for (int i = 0; i < TIMER_MAX; i++) {
    timer_t *tm = &g_timers[i];
    if (tm->active)
      continue; // 非空闲定时器
    tm->active = true;
    tm->cycle = cycle;
    tm->period = delay_ms;
    tm->last = g_heartbeat_count + delay_ms;
    tm->is_cb_mode = false; // 事件模式
    tm->mode.event.type = evt_type;
    return i;
  }
  return -1; // 定时器满
}

timer_id_t timer_add_cb(uint32_t delay_ms, timer_cb_t cb, void *arg,
                        bool cycle) {
  for (int i = 0; i < TIMER_MAX; i++) {
    timer_t *tm = &g_timers[i];
    if (tm->active)
      continue; // 非空闲定时器
    tm->active = true;
    tm->cycle = cycle;
    tm->period = delay_ms;
    tm->last = g_heartbeat_count + delay_ms;
    tm->is_cb_mode = true; // 回调模式
    tm->mode.callback.cb = cb;
    tm->mode.callback.arg = arg;
    return i;
  }
  return -1;
}

void timer_del(timer_id_t timer_id) {
  if (timer_id < 0 || timer_id >= TIMER_MAX)
    return;
  g_timers[timer_id].active = false;
}

void timer_clear(void) {
  for (int i = 0; i < TIMER_MAX; i++) {
    g_timers[i].active = false;
  }
}

void timer_tick(void) {
  if (g_tick_flag <= 0)
    return;
  g_tick_flag--;
  for (int i = 0; i < TIMER_MAX; i++) {
    timer_t *tm = &g_timers[i];
    if (!tm->active)
      continue; // 未定时

    if (tm->last > g_heartbeat_count)
      continue; // 未超时

    // 定时时间到
    if (tm->is_cb_mode) {
      // 回调模式, 直接同步调用
      timer_cb_t cb = tm->mode.callback.cb;
      void *arg = tm->mode.callback.arg;
      if (cb) {
        cb(arg);
      }
    } else {
      // 事件模式, 发送异步事件
      event_push(&tm->mode.event);
    }

    if (tm->cycle) {
      // 周期定时器, 重启
      tm->last = g_heartbeat_count + tm->period;
    } else {
      // 单发定时器, 停止
      tm->active = false;
    }
  }
}

void timer_delay_ms(uint32_t ms) {
  volatile uint32_t last = g_heartbeat_count + ms;
  while (g_heartbeat_count < last)
    ;
}

void hartbeat_handler(void) {
  g_heartbeat_count++; // 每次心跳, +1
  g_tick_flag++;
  // timer_tick();
}
