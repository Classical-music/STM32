#include "queue.h"
#include <string.h>

void queue_init(queue_t *q, uint8_t *buff, const uint32_t elem_size,
                const uint32_t buff_size) {
  if (q == NULL || buff == NULL)
    return;
  q->buff = buff;
  q->elem_size = elem_size;
  q->buff_size = buff_size;
  q->read = q->write = 0;
}
bool queue_push(queue_t *q, const uint8_t *data) {
  if (q == NULL || data == NULL)
    return false;
  if (queue_isfull(q))
    return false; // 队列已满
  memcpy(q->buff + q->write, data, q->elem_size);
  q->write = (q->write + q->elem_size) % q->buff_size;
  return true;
}
bool queue_pop(queue_t *q, uint8_t *data) {
  if (q == NULL || data == NULL)
    return false;
  if (queue_isempty(q))
    return false; // 队列空
  memcpy(data, q->buff + q->read, q->elem_size);
  q->read = (q->read + q->elem_size) % q->buff_size;
  return true;
}
bool queue_isfull(const queue_t *q) {
  return (q->write + q->elem_size) % q->buff_size == q->read;
}
bool queue_isempty(const queue_t *q) { return q->read == q->write; }
void queue_clear(queue_t *q) { q->read = q->write = 0; }
