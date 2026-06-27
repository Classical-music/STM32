#ifndef __I2C_H__
#define __I2C_H__

#include "type.h"

typedef struct {
  I2C_TypeDef *i2c;

  uint32_t freq;
  bool fast; // 快速模式
  bool duty; // 占空比
} i2c_t;

void i2c_init(i2c_t *ctx);

bool i2c_start(i2c_t *ctx);

void i2c_stop(i2c_t *ctx);

bool i2c_send_byte(i2c_t *ctx, uint8_t data);

uint8_t i2c_read_byte(i2c_t *ctx, bool ack);

#define I2C_INIT(ctx) i2c_init(&ctx)
#define I2C_START(ctx) i2c_start(&ctx)
#define I2C_STOP(ctx) i2c_stop(&ctx)
#define I2C_SEND_ADDR(ctx, addr) i2c_send_byte(&ctx, addr)
#define I2C_SEND_BYTE(ctx, data) i2c_send_byte(&ctx, data)
#define I2C_READ_BYTE(ctx, ack) i2c_read_byte(&ctx, ack)

#endif
