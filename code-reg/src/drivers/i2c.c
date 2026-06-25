#include "i2c.h"

#define CHECK_TIMEOUT(cond)                                                    \
  do {                                                                         \
    uint16_t timeout = 0xFFFF;                                                 \
    while ((cond) && timeout > 0)                                              \
      timeout--;                                                               \
    if (timeout == 0)                                                          \
      return false;                                                            \
  } while (0)

bool i2c_init(i2c_t *i2c) {
  // RCC
  RCC->APB1ENR |= i2c->rcc;

  if (i2c->dma) { // 使能 DMA
    i2c->i2c->CR2 |= I2C_CR2_DMAEN;
  }

  i2c->i2c->SR2 |= I2C_SR2_MSL; // 主模式

  i2c->i2c->CCR = 0; // 清除ccr

  uint32_t pclk1 = SystemCoreClock / 2;
  uint32_t freq = pclk1 / 1000000;
  if (i2c->fast) { // 快速模式
    i2c->i2c->CCR |= I2C_CCR_FS;
    if (i2c->duty) { // 16/9
      i2c->i2c->CCR |= I2C_CCR_DUTY;
      i2c->i2c->CCR |= pclk1 / (25 * 400000);
    } else { // 2/1
      i2c->i2c->CCR |= freq / (3 * 400000);
    }
    i2c->i2c->TRISE = pclk1 + 1;
  } else { // 标准模式
    i2c->i2c->CCR |= pclk1 / (2 * 100000);
    i2c->i2c->TRISE = freq * 300 / 1000 + 1;
  }

  // 使能 I2C
  i2c->i2c->CR1 |= I2C_CR1_PE;
}

bool i2c_start(i2c_t *i2c) {
  // 等待总线空闲
  CHECK_TIMEOUT(i2c->i2c->SR2 & I2C_SR2_BUSY);

  i2c->i2c->CR1 |= I2C_CR1_START; // 发送起始信号

  // 等待起始信号发出
  CHECK_TIMEOUT(!(i2c->i2c->SR1 & I2C_SR1_SB));

  return true;
}

bool i2c_stop(i2c_t *i2c) {
  i2c->i2c->CR1 |= I2C_CR1_STOP; // 设置发送stop信号
}

bool i2c_is_addr(i2c_t *i2c) {
  return i2c->i2c->SR1 & I2C_SR1_ADDR; // 地址已发送
}

bool i2c_is_ack(i2c_t *i2c) {}

bool i2c_send_byte(i2c_t *i2c, uint8_t data) {
  while (!(i2c->i2c->SR1 & I2C_SR1_TXE))
    ; // 等待发送数据寄存器空
  i2c->i2c->DR = data;
}

uint8_t i2c_read_byte(i2c_t *i2c, bool ack) {
  while (!(i2c->i2c->SR1 & I2C_SR1_RXNE))
    ; // 等待接收数据寄存器非空
  uint8_t data = i2c->i2c->DR;
  if (ack) { // 设置ack
    i2c->i2c->CR1 |= I2C_CR1_ACK;
  } else { // 设置 nack
    i2c->i2c->CR1 &= ~I2C_CR1_ACK;
  }
  return data;
}
