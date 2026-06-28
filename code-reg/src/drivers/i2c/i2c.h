#ifndef __I2C_H__
#define __I2C_H__

#include "type.h"

#define I2C_HARD 1

#if I2C_HARD

typedef struct {
  I2C_TypeDef *i2c;

  uint32_t freq;
  bool fast; // 快速模式
  bool duty; // 占空比
} i2c_ctx_t;

#else

typedef struct {
  GPIO_TypeDef *scl_port;
  uint8_t scl_pin;
  GPIO_TypeDef *sda_port;
  uint8_t sda_pin;
} i2c_ctx_t;

#endif

typedef struct i2c_ops {
  void (*init)(i2c_ctx_t *ctx);
  bool (*start)(i2c_ctx_t *ctx);
  void (*stop)(i2c_ctx_t *ctx);
  bool (*send_addr)(i2c_ctx_t *ctx, uint8_t addr);
  bool (*send_byte)(i2c_ctx_t *ctx, uint8_t data);
  uint8_t (*read_byte)(i2c_ctx_t *ctx, bool ack);
} i2c_t;

extern i2c_t i2c_ops;

void i2c_init(i2c_ctx_t *ctx);

bool i2c_start(i2c_ctx_t *ctx);

void i2c_stop(i2c_ctx_t *ctx);

bool i2c_send_addr(i2c_ctx_t *ctx, uint8_t addr);

bool i2c_send_byte(i2c_ctx_t *ctx, uint8_t data);

uint8_t i2c_read_byte(i2c_ctx_t *ctx, bool ack);

#define I2C_INIT(ctx) i2c_init(&ctx)
#define I2C_START(ctx) i2c_start(&ctx)
#define I2C_STOP(ctx) i2c_stop(&ctx)
#define I2C_SEND_ADDR(ctx, addr) i2c_send_addr(&ctx, addr)
#define I2C_SEND_BYTE(ctx, data) i2c_send_byte(&ctx, data)
#define I2C_READ_BYTE(ctx, ack) i2c_read_byte(&ctx, ack)

#endif
