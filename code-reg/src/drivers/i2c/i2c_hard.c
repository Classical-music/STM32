#include "i2c.h"

#if I2C_HARD

/*
 * 硬件 I2C (STM32F1 I2C1/I2C2) 主机轮询实现.
 * 调用方要求:
 *   1. ctx 配置 i2c 外设与 freq(PCLK1, MHz), fast/duty 选择模式;
 *   2. scl/sda 引脚由 BSP 配置为 复用开漏(GPIO_CNF_O_Res_OD),
 *      且必须是外设对应引脚(如 I2C1 未重映射: PB6-SCL, PB7-SDA).
 */

/* 轮询超时上限, 72MHz 下约覆盖 100kHz 总线的多个字节时间 */
#define I2C_TIMEOUT 0x1FFFF

/* 等待 cond 成立, 超时则返回 retval */
#define I2C_WAIT(cond, retval)                                                 \
  do {                                                                         \
    uint32_t timeout = I2C_TIMEOUT;                                            \
    while (!(cond) && timeout > 0)                                             \
      timeout--;                                                               \
    if (timeout == 0)                                                          \
      return retval;                                                           \
  } while (0)

/* 清除 AF(应答失败) 标志: 向 SR1 对应位写 0 */
static void i2c_clear_af(I2C_TypeDef *i2c) {
  i2c->SR1 = (uint16_t)~I2C_SR1_AF;
}

static void hard_init(i2c_ctx_t *ctx) {
  I2C_TypeDef *i2c = ctx->i2c;
  uint32_t pclk = ctx->freq * 1000000; // PCLK1, Hz

  // 外设时钟在 APB1
  RCC->APB1ENR |= (i2c == I2C2) ? RCC_APB1ENR_I2C2EN : RCC_APB1ENR_I2C1EN;

  // 关闭 PE 后配置
  i2c->CR1 = 0;

  // CR2.FREQ = PCLK1 MHz 数
  i2c->CR2 = (i2c->CR2 & ~I2C_CR2_FREQ) | (ctx->freq & I2C_CR2_FREQ);

  // 波特率因子与上升时间
  if (ctx->fast) {
    i2c->CCR = I2C_CCR_FS;
    i2c->TRISE = ctx->freq * 300 / 1000 + 1; // 最大上升时间 300ns
    if (ctx->duty) {                         // 16/9
      i2c->CCR |= I2C_CCR_DUTY | (pclk / (10 * 400000));
    } else { // 2/1
      i2c->CCR |= pclk / (3 * 400000);
    }
  } else { // 标准模式 100kHz
    i2c->TRISE = ctx->freq + 1;              // 最大上升时间 1000ns
    i2c->CCR = pclk / (2 * 100000);
  }

  // 使能应答与外设
  i2c->CR1 = I2C_CR1_ACK | I2C_CR1_PE;
}

static bool hard_start(i2c_ctx_t *ctx) {
  I2C_TypeDef *i2c = ctx->i2c;

  // 上次传输以 STOP 结束后, BUSY 会保持到 STOPF 被清除, 先清 STOPF
  if (i2c->SR1 & I2C_SR1_STOPF) {
    (void)i2c->SR1;
    i2c->CR1 = i2c->CR1;
  }

  // 等待总线空闲
  I2C_WAIT(!(i2c->SR2 & I2C_SR2_BUSY), false);

  // 发送起始信号
  i2c->CR1 |= I2C_CR1_START;

  // 等待 SB 置位(轮询读 SR1 即完成 SB 标志的读清序列第一步)
  I2C_WAIT(i2c->SR1 & I2C_SR1_SB, false);

  return true;
}

static void hard_stop(i2c_ctx_t *ctx) {
  ctx->i2c->CR1 |= I2C_CR1_STOP;
}

static bool hard_send_addr(i2c_ctx_t *ctx, uint8_t addr) {
  I2C_TypeDef *i2c = ctx->i2c;

  // SB 后写 DR, 硬件发出地址 + R/W 位
  i2c->DR = addr;

  // 等待 ADDR(从机应答) 或 AF(应答失败)
  I2C_WAIT(i2c->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF), false);

  if (i2c->SR1 & I2C_SR1_AF) {
    i2c_clear_af(i2c);
    return false;
  }

  // 为接收传输准备应答(发送传输无副作用)
  i2c->CR1 |= I2C_CR1_ACK;

  // 读 SR1 + 读 SR2 清除 ADDR
  (void)i2c->SR1;
  (void)i2c->SR2;

  return true;
}

static bool hard_send_byte(i2c_ctx_t *ctx, uint8_t data) {
  I2C_TypeDef *i2c = ctx->i2c;

  // 等待数据寄存器空
  I2C_WAIT(i2c->SR1 & I2C_SR1_TXE, false);

  // 写入发送寄存器
  i2c->DR = data;

  // 等待该字节移位完成且应答位采样结束(不可读 DR 清 BTF, 会拉死 SCL)
  I2C_WAIT(i2c->SR1 & I2C_SR1_BTF, false);

  if (i2c->SR1 & I2C_SR1_AF) { // 从机 NACK
    i2c_clear_af(i2c);
    return false;
  }

  return true;
}

static uint8_t hard_read_byte(i2c_ctx_t *ctx, bool ack) {
  I2C_TypeDef *i2c = ctx->i2c;

  // 应答位须在字节接收前设置好
  if (ack) {
    i2c->CR1 |= I2C_CR1_ACK;
  } else { // 末字节: 先清 ACK, 收到后 NACK
    i2c->CR1 &= ~I2C_CR1_ACK;
  }

  // 等待接收寄存器非空
  I2C_WAIT(i2c->SR1 & I2C_SR1_RXNE, 0);

  if (!ack) {
    // 参考手册: 末字节须在读 DR 前置 STOP, 保证 NACK 后发出停止信号
    i2c->CR1 |= I2C_CR1_STOP;
  }

  return i2c->DR;
}

i2c_t i2c_ops_hard = {
    .init = hard_init,
    .start = hard_start,
    .stop = hard_stop,
    .send_addr = hard_send_addr,
    .send_byte = hard_send_byte,
    .read_byte = hard_read_byte,
};

#endif
