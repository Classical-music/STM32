#ifndef __FSM_H__
#define __FSM_H__

#include "event.h"

struct fsm_t; // 前向声明

typedef uint32_t state_t; // 状态类型

// 动作函数类型: 返回下一个状态,
typedef state_t (*action_t)(struct fsm_t *fsm, const event_t *ev);

// 转换表item
typedef struct {
  state_t current_state; // 当前状态
  event_id_t event_type; // 事件类型
  action_t action;       // 执行的动作
  state_t next_state;    // 转换后的状态
} transition_t;

// 状态机实例
typedef struct fsm_t {
  state_t current_state;     // 当前状态
  const transition_t *table; // 转换表指针
  size_t table_size;         // 转换表大小
} fsm_t;

void fsm_init(fsm_t *fsm, state_t state, const transition_t *table,
              size_t tbl_size);

void fsm_dispatch(fsm_t *fsm, const event_t *evt);

#define FSM_INIT(fsm, state, table)                                            \
  fsm_init(&fsm, state, table, ARRAY_SIZE(table))

#define FSM_DISPATCH(fsm, evt) fsm_dispatch(&fsm, evt)

#endif
