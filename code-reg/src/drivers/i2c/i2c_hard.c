#include "i2c.h"

#if I2C_HARD

#define CHECK_TIMEOUT(cond)                                                    \
  do {                                                                         \
    uint16_t timeout = 0xFFFF;                                                 \
    while ((cond) && timeout > 0)                                              \
      timeout--;                                                               \
    if (timeout == 0)                                                          \
      return false;                                                            \
  } while (0)

static void i2c_init(i2c_ctx_t *ctx) {
  ctx->i2c->CR1 &= ~I2C_CR1_SMBUS; // I2C模式

  // i2c 时钟频率
  ctx->i2c->CR2 &= ~I2C_CR2_FREQ;
  ctx->i2c->CR2 |= ctx->freq;

  // 根据 fs & duty 配置 CCR & TRISE
  ctx->i2c->CCR = 0; // 清除ccr
  if (ctx->fast) {   // 快速模式
    ctx->i2c->CCR |= I2C_CCR_FS;
    ctx->i2c->TRISE = ctx->freq + 1;
    if (ctx->duty) { // 16/9
      ctx->i2c->CCR |= I2C_CCR_DUTY;
      ctx->i2c->CCR |= (ctx->freq * 1000000) / (25 * 400000);
    } else { // 2/1
      ctx->i2c->CCR |= (ctx->freq * 1000000) / (3 * 400000);
    }
  } else { // 标准模式
    ctx->i2c->TRISE = ctx->freq * 300 / 1000 + 1;
    ctx->i2c->CCR |= (ctx->freq * 1000000) / (2 * 100000);
  }

  // 使能 I2C
  ctx->i2c->CR1 |= I2C_CR1_PE;
}

static bool i2c_start(i2c_ctx_t *ctx) {
  // 检测总线忙
  CHECK_TIMEOUT(ctx->i2c->SR2 & I2C_SR2_BUSY);

  ctx->i2c->CR1 |= I2C_CR1_START; // 发送起始信号

  // 等待起始信号发出
  CHECK_TIMEOUT(!(ctx->i2c->SR1 & I2C_SR1_SB));

  return true;
}

static void i2c_stop(i2c_ctx_t *ctx) {
  ctx->i2c->CR1 |= I2C_CR1_STOP; // 设置发送stop信号
}

static bool i2c_send_addr(i2c_ctx_t *ctx, uint8_t addr) {
  // 数据发送到 DR
  ctx->i2c->DR = addr;
  // 等待应答
  CHECK_TIMEOUT(!(ctx->i2c->SR1 & I2C_SR1_ADDR));
  ctx->i2c->SR2; // 访问SR2, 清除 ADDR

  return true;
}

static bool i2c_send_byte(i2c_ctx_t *ctx, uint8_t data) {
  // 等待发送DR空
  CHECK_TIMEOUT(!(ctx->i2c->SR1 & I2C_SR1_TXE));

  // 写数据, 准备发送
  ctx->i2c->DR = data;

  // 等待应答
  CHECK_TIMEOUT(!(ctx->i2c->SR1 & I2C_SR1_BTF));
  ctx->i2c->DR; // 访问DR, 清除 BTF

  return true;
}

static uint8_t i2c_read_byte(i2c_ctx_t *ctx, bool ack) {
  // 等待接收DR非空
  CHECK_TIMEOUT(!(ctx->i2c->SR1 & I2C_SR1_RXNE));

  if (ack) { // 设置ack
    ctx->i2c->CR1 |= I2C_CR1_ACK;
  } else { // 设置 nack
    ctx->i2c->CR1 &= ~I2C_CR1_ACK;
  }

  return ctx->i2c->DR;
}

i2c_t i2c_ops = {
    .init = i2c_init,
    .start = i2c_start,
    .stop = i2c_stop,
    .send_addr = i2c_send_addr,
    .send_byte = i2c_send_byte,
    .read_byte = i2c_read_byte,
};

#endif
