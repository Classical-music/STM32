#ifndef TASK_BEEP_H
#define TASK_BEEP_H

#include <stdint.h>

static inline void beepTaskInit(void) {}
static inline void playBeep(uint32_t period_ms) {
  (void)period_ms;
}

#define playAlert() playBeep(20)

#endif
