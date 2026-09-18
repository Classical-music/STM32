#ifndef __I2C_H__
#define __I2C_H__

#include "type.h"

/*
 * I2C_HARD: 0-软件模拟 I2C, 1-允许使用硬件 I2C (可通过编译期 -D 覆盖)
 *
 * 软件总线(仅配置 GPIO 引脚)在任何配置下都可用; 硬件总线需要 ctx 配置
 * i2c 外设(I2C1/I2C2), 且引脚必须是对应外设的复用引脚(如 I2C1: PB6/PB7),
 * 仅当 I2C_HARD=1 时生效, 否则退回软件模拟。
 */
#ifndef I2C_HARD
#define I2C_HARD 1
#endif

typedef struct {
  GPIO_TypeDef *scl_port;
  uint8_t scl_pin;
  GPIO_TypeDef *sda_port;
  uint8_t sda_pin;

#if I2C_HARD
  I2C_TypeDef *i2c; // 非 NULL 且引脚为硬件复用引脚时走硬件 I2C
  uint32_t freq;    // I2C 外设输入时钟 PCLK1, 单位 MHz
  bool fast;        // 快速模式(400kHz)
  bool duty;        // 快速模式占空比 16/9 (false 为 2/1)
#endif
} i2c_ctx_t;

typedef struct i2c_ops {
  void (*init)(i2c_ctx_t *ctx);
  bool (*start)(i2c_ctx_t *ctx);
  void (*stop)(i2c_ctx_t *ctx);
  bool (*send_addr)(i2c_ctx_t *ctx, uint8_t addr);
  bool (*send_byte)(i2c_ctx_t *ctx, uint8_t data);
  uint8_t (*read_byte)(i2c_ctx_t *ctx, bool ack);
} i2c_t;

extern i2c_t i2c_ops_soft;
#if I2C_HARD
extern i2c_t i2c_ops_hard;
#endif

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
