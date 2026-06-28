#include "i2c.h"

static i2c_t *i2c_ptr = &i2c_ops;

void i2c_init(i2c_ctx_t *ctx) {
  i2c_ptr->init(ctx); //
}

bool i2c_start(i2c_ctx_t *ctx) {
  return i2c_ptr->start(ctx); //
}

void i2c_stop(i2c_ctx_t *ctx) {
  i2c_ptr->stop(ctx); //
}

bool i2c_send_addr(i2c_ctx_t *ctx, uint8_t addr) {
  return i2c_ptr->send_addr(ctx, addr); //
}

bool i2c_send_byte(i2c_ctx_t *ctx, uint8_t data) {
  return i2c_ptr->send_byte(ctx, data); //
}

uint8_t i2c_read_byte(i2c_ctx_t *ctx, bool ack) {
  return i2c_ptr->read_byte(ctx, ack); //
}
