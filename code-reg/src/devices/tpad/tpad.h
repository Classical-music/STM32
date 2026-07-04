#ifndef __TPAD_H__
#define __TPAD_H__

#include "key/key.h"
#include "tim.h"

enum {
  KEY_TPAD = KEY_NUM,
};

void tpad_init(void);
void tpad_reset(void);
bool tpad_is_press(void);

#endif
