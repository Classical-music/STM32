#ifndef __TIMER_H__
#define __TIMER_H__

#include "event.h"

typedef void (*timer_cb_t)(void *arg);
typedef int32_t timer_id_t; // 定时器id

// 定时器结构体
typedef struct {
  bool active;     // 是否激活
  bool cycle;      // 单次/周期
  uint32_t period; // 周期
  uint32_t last;   // 结束时间

  union {
    event_t event; // 事件模式
    struct {
      timer_cb_t cb;
      void *arg;
    } callback; // 回调模式
  } mode;

  bool is_cb_mode;
} timer_t;

// 初始化定时器模块(需要在 systick 启动前调用)
void timer_init(void);
// 启动一个定时器, 返回定时器 id
timer_id_t timer_add(uint32_t delay_ms, bool cycle, event_id_t evt_type);
timer_id_t timer_add_cb(uint32_t delay_ms, timer_cb_t cb, void *arg,
                        bool cycle);
void timer_del(timer_id_t timer_id);
void timer_clear(void);

void timer_tick(void);

void timer_delay_ms(uint32_t ms);

#define TIMER_ADD_CB(delay_ms, cb, arg, cycle)                                 \
  timer_add_cb((uint32_t)delay_ms, cb, (void *)arg, cycle)

#endif
