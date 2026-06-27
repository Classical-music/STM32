#include "gpio.h"
#include "i2c.h"

#define SCL_HIGH(i2c) GPIO_HIGH(GPIOB, 6)
#define SCL_LOW(i2c) GPIO_LOW(GPIOB, 6)
#define SDA_HIGH(i2c) GPIO_HIGH(GPIOB, 7)
#define SDA_LOW(i2c) GPIO_LOW(GPIOB, 7)

#define SDA_READ(i2c) GPIO_READ(GPIOB, 7)

#define I2C_DELAY                                                              \
  do {                                                                         \
    uint16_t i = 0xFF;                                                         \
    while (i--)                                                                \
      ;                                                                        \
  } while (0)

void i2c_init(i2c_t *ctx) {}

bool i2c_start(i2c_t *ctx) {
  // 准备
  SDA_HIGH(ctx);
  SCL_HIGH(ctx);
  I2C_DELAY;

  // 产生 Start 跳变
  SDA_LOW(ctx);
  I2C_DELAY;

  return true;
}

void i2c_stop(i2c_t *ctx) {
  // 准备
  SDA_LOW(ctx);
  SCL_HIGH(ctx);
  I2C_DELAY;

  // 产生 Stop 跳变
  SDA_HIGH(ctx);
  I2C_DELAY;
}

static void i2c_ack(bool ack) {
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

static bool i2c_wait_ack(i2c_t *ctx) {
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
bool i2c_send_byte(i2c_t *ctx, uint8_t data) {
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

uint8_t i2c_read_byte(i2c_t *ctx, bool ack) {
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
  i2c_ack(ack);

  return data;
}
