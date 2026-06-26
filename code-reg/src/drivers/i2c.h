#ifndef __I2C_H__
#define __I2C_H__

#include "type.h"

typedef struct {
  I2C_TypeDef *i2c;

  uint32_t freq;
  bool fast; // 快速模式
  bool duty; // 占空比
} i2c_t;

void i2c_init(i2c_t *i2c);

bool i2c_start(i2c_t *i2c);

void i2c_stop(i2c_t *i2c);

// 发送I2C设备地址
bool i2c_send_addr(i2c_t *i2c, uint8_t addr);

bool i2c_send_byte(i2c_t *i2c, uint8_t data);

uint8_t i2c_read_byte(i2c_t *i2c, bool ack);

#endif
