#ifndef __TASK_KEY_H__
#define __TASK_KEY_H__

#include "key/key.h"
#include "queue.h"
#include "type.h"

enum {
  KEY_EVT_PRESSED = 0,   // 按下
  KEY_EVT_RELEASED,      // 松开
  KEY_EVT_LONG_PRESSED,  // 长按
  KEY_EVT_LONG_RELEASED, // 长松
  KEY_EVT_HOLD,          // hold
};

typedef struct {
  uint8_t keyId;   // 按键ID
  uint8_t evtType; // 事件类型
} KeyMsg_t;

extern QueueHandle_t xKeyQue;
#define QUEUE_KEY_SEND(id, evt)                                                \
  do {                                                                         \
    KeyMsg_t xMsg = {id, evt};                                                 \
    xQueueSend(xKeyQue, &xMsg, pdMS_TO_TICKS(10));                             \
  } while (0);

void keyTaskInit(void);

#endif
