#ifndef __TASK_CONSOLE_H__
#define __TASK_CONSOLE_H__

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "usart/usart.h"

extern MessageBufferHandle_t xConsoleBuff;

void taskConsoleInit(void);

#endif
