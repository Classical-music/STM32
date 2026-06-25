#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "type.h"

typedef struct {
  uint8_t *buff;
  uint32_t elem_size;
  uint32_t buff_size;
  volatile uint32_t read;
  volatile uint32_t write;
} queue_t;

void queue_init(queue_t *q, uint8_t *buff, const uint32_t elem_size,
                const uint32_t buff_size);
bool queue_push(queue_t *q, const uint8_t *data);
bool queue_pop(queue_t *q, uint8_t *data);
bool queue_isfull(const queue_t *q);
bool queue_isempty(const queue_t *q);
void queue_clear(queue_t *q);

#define QUEUE_INIT(q, buff)                                                    \
  queue_init(&q, (uint8_t *)buff, sizeof(buff[0]), sizeof(buff))
#define QUEUE_PUSH(q, data) queue_push(&q, (uint8_t *)&data)
#define QUEUE_POP(q, data) queue_pop(&q, (uint8_t *)&data)
#define QUEUE_ISFULL(q) queue_isfull(&q)
#define QUEUE_ISEMPTY(q) queue_isempty(&q)
#define QUEUE_CLEAR(q) queue_clear(&q)

#endif
