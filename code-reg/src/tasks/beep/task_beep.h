#ifndef __TASK_BEEP_H__
#define __TASK_BEEP_H__

#include "type.h"

void beepTaskInit(void);

void playBeep(uint32_t period_ms);

#define playAlert() playBeep(20)

#endif
