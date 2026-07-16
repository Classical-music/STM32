#ifndef __TASK_TOUCH_H__
#define __TASK_TOUCH_H__

#include "FreeRTOS.h"
#include "semphr.h"
#include "type.h"

extern SemaphoreHandle_t xTouchSemaph;

void taskTouchInit(void);

bool taskTouchIsPressed(void);

void taskTouchGetPos(uint16_t *x, uint16_t *y);

#endif
