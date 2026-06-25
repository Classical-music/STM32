#ifndef __I2C_H__
#define __I2C_H__

#include "type.h"

#define I2C_FREQ_MASK 0x3F

// CR1
// SWRST, 软件复位

// SR
// STOPF 停止位检测

typedef struct {
  I2C_TypeDef *i2c;
  uint16_t rcc;

  bool dma;  // 是否启用dma
  bool fast; // 快速模式
  bool duty; // 占空比
} i2c_t;

bool i2c_init(i2c_t *i2c);

bool i2c_start(i2c_t *i2c);

bool i2c_stop(i2c_t *i2c);

bool i2c_is_addr(i2c_t *i2c);

bool i2c_is_ack(i2c_t *i2c);

bool i2c_send_byte(i2c_t *i2c, uint8_t data);

uint8_t i2c_read_byte(i2c_t *i2c, bool ack);

#endif
