#include "i2c.h"

#if !I2C_HARD

#include "gpio.h"

#define SCL_HIGH(ctx) GPIO_HIGH(ctx->scl_port, ctx->scl_pin)
#define SCL_LOW(ctx) GPIO_LOW(ctx->scl_port, ctx->scl_pin)
#define SDA_HIGH(ctx) GPIO_HIGH(ctx->sda_port, ctx->sda_pin)
#define SDA_LOW(ctx) GPIO_LOW(ctx->sda_port, ctx->sda_pin)

#define SDA_READ(ctx) GPIO_READ(ctx->sda_port, ctx->sda_pin)

#define I2C_DELAY                                                              \
  do {                                                                         \
    uint16_t i = 0x8;                                                          \
    while (i--)                                                                \
      ;                                                                        \
  } while (0)

static void i2c_init(i2c_ctx_t *ctx) {}

static bool i2c_start(i2c_ctx_t *ctx) {
  // 准备
  SDA_HIGH(ctx);
  SCL_HIGH(ctx);
  I2C_DELAY;

  // 产生 Start 跳变
  SDA_LOW(ctx);
  I2C_DELAY;

  return true;
}

static void i2c_stop(i2c_ctx_t *ctx) {
  // 准备
  SDA_LOW(ctx);
  SCL_HIGH(ctx);
  I2C_DELAY;

  // 产生 Stop 跳变
  SDA_HIGH(ctx);
  I2C_DELAY;
}

static void i2c_ack(i2c_ctx_t *ctx, bool ack) {
  // 准备
  SCL_LOW(ctx);
  SDA_HIGH(ctx);
  I2C_DELAY;

  if (ack) {
    // 发出 ack 应答
    SDA_LOW(ctx);
    I2C_DELAY;
  }
  // nack SDA 保持高电平

  // 拉高SCL, 采样
  SCL_HIGH(ctx);
  I2C_DELAY;
  // 拉低SCL, 采样结束
  SCL_LOW(ctx);
  I2C_DELAY;

  if (ack) {
    // 释放总线
    SDA_HIGH(ctx);
    I2C_DELAY;
  }
  // nack, SDA 无需释放
}

static bool i2c_wait_ack(i2c_ctx_t *ctx) {
  // 释放数据总线
  SCL_LOW(ctx);
  SDA_HIGH(ctx);
  I2C_DELAY;

  // SCL 拉高, 开始采样
  SCL_HIGH(ctx);
  I2C_DELAY;

  // 读取 SDA
  uint16_t ack = SDA_READ(ctx);

  // SCL 拉低, 采样结束
  SCL_LOW(ctx);
  I2C_DELAY;

  return !ack;
}

// 发送一个字节
static bool i2c_send_byte(i2c_ctx_t *ctx, uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    // 拉低 SCL, 准备发数据
    SCL_LOW(ctx);
    SDA_LOW(ctx);
    I2C_DELAY;

    // 发送数据
    if (data & 0x80) { // 高位先发
      SDA_HIGH(ctx);   // 发送1
    }
    // 发送0, 无需再拉低 SDA
    data <<= 1;
    I2C_DELAY;

    // 拉高 SCL, 准备采样
    SCL_HIGH(ctx);
    I2C_DELAY;
  }

  // 读取, 并返回 ack/nack
  return i2c_wait_ack(ctx);
}

static uint8_t i2c_read_byte(i2c_ctx_t *ctx, bool ack) {
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    // SCL 拉低, 从机准备发数据
    SCL_LOW(ctx);
    I2C_DELAY;

    // SCL 拉高, 准备采样数据
    SCL_HIGH(ctx);
    I2C_DELAY;

    data <<= 1;
    if (SDA_READ(ctx)) { // 读到1
      data |= 0x01;
    }
    // 读到0, 无需赋值
  }

  // 发送 ack/nack 信号
  i2c_ack(ctx, ack);

  return data;
}

i2c_t i2c_ops = {
    .init = i2c_init,
    .start = i2c_start,
    .stop = i2c_stop,
    .send_addr = i2c_send_byte,
    .send_byte = i2c_send_byte,
    .read_byte = i2c_read_byte,
};

#endif
