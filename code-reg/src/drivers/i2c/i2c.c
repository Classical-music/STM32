#include "i2c.h"

/*
 * 包装层: 按 ctx 选择具体实现.
 * I2C_HARD=1 时, 配置了 i2c 外设的总线走硬件实现, 其余(仅 GPIO 引脚)
 * 仍走软件模拟; I2C_HARD=0 时全部走软件模拟.
 */
static i2c_t *i2c_select(i2c_ctx_t *ctx) {
#if I2C_HARD
  if (ctx->i2c) {
    return &i2c_ops_hard;
  }
#else
  (void)ctx;
#endif
  return &i2c_ops_soft;
}

void i2c_init(i2c_ctx_t *ctx) { i2c_select(ctx)->init(ctx); }

bool i2c_start(i2c_ctx_t *ctx) { return i2c_select(ctx)->start(ctx); }

void i2c_stop(i2c_ctx_t *ctx) { i2c_select(ctx)->stop(ctx); }

bool i2c_send_addr(i2c_ctx_t *ctx, uint8_t addr) {
  return i2c_select(ctx)->send_addr(ctx, addr);
}

bool i2c_send_byte(i2c_ctx_t *ctx, uint8_t data) {
  return i2c_select(ctx)->send_byte(ctx, data);
}

uint8_t i2c_read_byte(i2c_ctx_t *ctx, bool ack) {
  return i2c_select(ctx)->read_byte(ctx, ack);
}
