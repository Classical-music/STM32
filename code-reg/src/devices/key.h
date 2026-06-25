#ifndef __KEY_H__
#define __KEY_H__

#include "type.h"

typedef enum {
  KEY_START = 0,
  KEY_UP = KEY_START, // PA0
  KEY_DOWN,           // PE3
  KEY_LEFT,           // PE2
  KEY_RIGHT,          // PE4
  KEY_NUM,
} key_id_t;

void key_init(void);

bool key_ispress(key_id_t key);

#endif
