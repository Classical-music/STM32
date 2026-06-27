#include "i2c.h"

#define CHECK_TIMEOUT(cond)                                                    \
  do {                                                                         \
    uint16_t timeout = 0xFFFF;                                                 \
    while ((cond) && timeout > 0)                                              \
      timeout--;                                                               \
    if (timeout == 0)                                                          \
      return false;                                                            \
  } while (0)

void i2c_init(i2c_t *i2c) {
  i2c->i2c->CR1 &= ~I2C_CR1_SMBUS; // I2C模式

  // i2c 时钟频率
  i2c->i2c->CR2 &= ~I2C_CR2_FREQ;
  i2c->i2c->CR2 |= i2c->freq;

  // 根据 fs & duty 配置 CCR & TRISE
  i2c->i2c->CCR = 0; // 清除ccr
  if (i2c->fast) {   // 快速模式
    i2c->i2c->CCR |= I2C_CCR_FS;
    i2c->i2c->TRISE = i2c->freq + 1;
    if (i2c->duty) { // 16/9
      i2c->i2c->CCR |= I2C_CCR_DUTY;
      i2c->i2c->CCR |= (i2c->freq * 1000000) / (25 * 400000);
    } else { // 2/1
      i2c->i2c->CCR |= (i2c->freq * 1000000) / (3 * 400000);
    }
  } else { // 标准模式
    i2c->i2c->TRISE = i2c->freq * 300 / 1000 + 1;
    i2c->i2c->CCR |= (i2c->freq * 1000000) / (2 * 100000);
  }

  // 使能 I2C
  i2c->i2c->CR1 |= I2C_CR1_PE;
}

bool i2c_start(i2c_t *i2c) {
  // 检测总线忙
  CHECK_TIMEOUT(i2c->i2c->SR2 & I2C_SR2_BUSY);

  i2c->i2c->CR1 |= I2C_CR1_START; // 发送起始信号

  // 等待起始信号发出
  CHECK_TIMEOUT(!(i2c->i2c->SR1 & I2C_SR1_SB));

  return true;
}

void i2c_stop(i2c_t *i2c) {
  i2c->i2c->CR1 |= I2C_CR1_STOP; // 设置发送stop信号
}

bool i2c_send_addr(i2c_t *i2c, uint8_t addr) {
  // 数据发送到 DR
  i2c->i2c->DR = addr;
  // 等待应答
  CHECK_TIMEOUT(!(i2c->i2c->SR1 & I2C_SR1_ADDR));
  i2c->i2c->SR2; // 访问SR2, 清除 ADDR

  return true;
}

bool i2c_send_byte(i2c_t *i2c, uint8_t data) {
  // 等待发送DR空
  CHECK_TIMEOUT(!(i2c->i2c->SR1 & I2C_SR1_TXE));

  // 写数据, 准备发送
  i2c->i2c->DR = data;

  // 等待应答
  CHECK_TIMEOUT(!(i2c->i2c->SR1 & I2C_SR1_BTF));
  i2c->i2c->DR; // 访问DR, 清除 BTF

  return true;
}

uint8_t i2c_read_byte(i2c_t *i2c, bool ack) {
  // 等待接收DR非空
  CHECK_TIMEOUT(!(i2c->i2c->SR1 & I2C_SR1_RXNE));

  if (ack) { // 设置ack
    i2c->i2c->CR1 |= I2C_CR1_ACK;
  } else { // 设置 nack
    i2c->i2c->CR1 &= ~I2C_CR1_ACK;
  }

  return i2c->i2c->DR;
}
