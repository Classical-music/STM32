#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "usart.h"

void console_init(void);

uint32_t console_read_buff(uint8_t *buff, uint32_t size);

#define CONSOLE_READ_BUFF(buff, size) console_read_buff((uint8_t *)buff, size)

#endif
