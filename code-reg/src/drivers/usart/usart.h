#ifndef __USART_H__
#define __USART_H__

#include "type.h"

void usart_init(uint32_t baud);
void usart_send_byte(uint8_t data);
bool usart_read_byte(uint8_t *data);

#endif
