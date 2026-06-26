#include "eeprom.h"
#include "gpio.h"
#include "i2c.h"
#include "timer.h"

/**
 * 24c02 相关参数
 * 1. I2C 引脚 PB6-SCL & PB7-SDA
 * 2. I2C1 未重映射
 * 3. 24c02 地址: 1010000 | 0/1 写/读
 */

#define RADDR_24C02 0xA1
#define WADDR_24C02 0xA0

static i2c_t i2c1 = {.i2c = I2C1, .freq = 36, .fast = false, .duty = false};

static void init_24c02(void) {
  // RCC
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  // GPIO 模式 PB6&PB7-复用开漏输出
  GPIO_MODE_SET(GPIOB, 6, GPIO_MODE_O50 | GPIO_CNF_O_Res_OD);
  GPIO_MODE_SET(GPIOB, 7, GPIO_MODE_O50 | GPIO_CNF_O_Res_OD);

  // 初始化 I2C
  i2c_init(&i2c1);
}

static uint8_t read_byte_24c02(uint8_t addr) {
  // 发出 start 信号
  if (!i2c_start(&i2c1))
    return 0;

  // 发送写地址
  if (!i2c_send_addr(&i2c1, WADDR_24C02)) {
    return 0;
  }

  // 发送内部地址
  if (!i2c_send_byte(&i2c1, addr)) {
    return 0;
  }

  // 发送读地址
  if (!i2c_send_addr(&i2c1, RADDR_24C02)) {
    return 0;
  }

  // 读取一个字节
  uint8_t byte = i2c_read_byte(&i2c1, false);

  // 发送停止信号
  i2c_stop(&i2c1);

  return byte;
}

static void write_byte_24c02(uint8_t addr, uint8_t data) {
  // 发出 start 信号
  if (!i2c_start(&i2c1))
    return;

  // 发送写地址
  if (!i2c_send_addr(&i2c1, WADDR_24C02)) {
    return;
  }

  // 发送内部地址
  if (!i2c_send_byte(&i2c1, addr)) {
    return;
  }

  // 发送数据
  if (!i2c_send_byte(&i2c1, data)) {
    return;
  }

  // 发送停止信号
  i2c_stop(&i2c1);

  // 等待5ms
  timer_delay_ms(5);
}

void read_24c02(uint8_t addr, uint8_t *buff, uint8_t size) {
  // 发出 start 信号
  if (!i2c_start(&i2c1))
    return;

  // 发送写地址
  if (!i2c_send_addr(&i2c1, WADDR_24C02)) {
    return;
  }

  // 发送内部地址
  if (!i2c_send_byte(&i2c1, addr)) {
    return;
  }

  // 发送读地址
  if (!i2c_send_addr(&i2c1, RADDR_24C02)) {
    return;
  }

  for (uint8_t i = 0; i < size; i++) {
    buff[i] = i2c_read_byte(&i2c1, i < size - 1);
  }

  // 发送停止信号
  i2c_stop(&i2c1);
}

void write_24c02(uint8_t addr, uint8_t *buff, uint8_t size) {
  // 发出 start 信号
  if (!i2c_start(&i2c1))
    return;

  // 发送写地址
  if (!i2c_send_addr(&i2c1, WADDR_24C02)) {
    return;
  }

  // 发送内部地址
  if (!i2c_send_byte(&i2c1, addr)) {
    return;
  }

  for (uint8_t i = 0; i < size; i++) {
    if (!i2c_send_byte(&i2c1, buff[i])) {
      break;
    }
  }

  // 发送停止信号
  i2c_stop(&i2c1);

  // 等待5ms
  timer_delay_ms(5);
}

eeprom_t eeprom_24c02 = {
    .init = init_24c02,
    .read_byte = read_byte_24c02,
    .write_byte = write_byte_24c02,
    .read = read_24c02,
    .write = NULL,
};
