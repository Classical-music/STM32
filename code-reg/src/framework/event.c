#include "event.h"
#include "queue.h"

// 事件队列
#define EVT_BUFF_SIZE 64
static event_t evt_buff[EVT_BUFF_SIZE];
static queue_t evt_que;

static event_handler_t g_handlers[EVT_ID_MAX] = {NULL};

void event_register(event_id_t type, event_handler_t cb) {
  g_handlers[type] = cb;
}

void event_init(void) { QUEUE_INIT(evt_que, evt_buff); }

bool event_push(event_t *e) { return QUEUE_PUSH(evt_que, *e); }

void event_dispatch(void) {
  event_t evt;
  while (QUEUE_POP(evt_que, evt)) {               // 取事件
    event_id_t evt_id = evt.type;                 // 事件类型
    event_handler_t handler = g_handlers[evt_id]; // 事件回调
    if (!handler)
      continue;
    handler(&evt); // 执行事件
  }
}
