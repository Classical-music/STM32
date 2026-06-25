#ifndef __EVENT_H__
#define __EVENT_H__

#include "type.h"

enum {
  EVT_ID_START = 0,

  // 按键
  EVT_ID_KEY_PRESSED,   // 短按
  EVT_ID_KEY_RELEASED,  // 短松
  EVT_ID_KEY_LPRESSED,  // 长按
  EVT_ID_KEY_LRELEASED, // 长松
  EVT_ID_KEY_HOLD,      // 持续按

  // 命令行解析
  EVT_ID_CMDLINE_READY,

  EVT_ID_MAX,
};

typedef uint32_t event_id_t;

typedef struct {
  event_id_t type;
  uint32_t param;
  void *data;
} event_t;

typedef void (*event_handler_t)(const event_t *);

void event_register(event_id_t type, event_handler_t cb);

void event_init(void);
bool event_push(event_t *e);

void event_dispatch(void);

#endif
