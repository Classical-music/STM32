#include "eeprom.h"
#include "gpio.h"
#include "i2c/i2c.h"
#include "timer.h"

/**
 * 24c02 相关参数
 * 1. I2C 引脚 PB6-SCL & PB7-SDA
 * 2. I2C1 未重映射
 * 3. 24c02 地址: 1010000 | 0/1 写/读
 * 4. 使用软件模拟I2C
 */

#define RADDR_24C02 0xA1
#define WADDR_24C02 0xA0

#define DELAY_EEPROM timer_delay_ms(5)

#if I2C_HARD

static i2c_ctx_t i2c_ctx = {
    .i2c = I2C1, .freq = 36, .fast = false, .duty = false};

#else

static i2c_ctx_t i2c_ctx = {
    .scl_port = GPIOB,
    .scl_pin = 6,
    .sda_port = GPIOB,
    .sda_pin = 7,
};

#endif

static void init_24c02(void) {
  // RCC
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  // GPIO 模式 PB6&PB7-通用开漏输出
  GPIO_MODE_SET(GPIOB, 6, GPIO_MODE_O2 | GPIO_CNF_O_Gen_OD);
  GPIO_MODE_SET(GPIOB, 7, GPIO_MODE_O2 | GPIO_CNF_O_Gen_OD);

  // 初始化 I2C
  I2C_INIT(i2c_ctx);
}

static uint8_t read_byte_24c02(uint8_t addr) {
  // 发送起始信号
  if (!I2C_START(i2c_ctx)) {
    printf("%s:%d i2c_start failed\n", __FILE__, __LINE__);
    return 0;
  }

  // 发送设备地址(写)
  if (!I2C_SEND_ADDR(i2c_ctx, WADDR_24C02)) {
    I2C_STOP(i2c_ctx);
    return 0;
  }
  // 发送内部地址
  if (!I2C_SEND_BYTE(i2c_ctx, addr)) {
    I2C_STOP(i2c_ctx);
    return 0;
  }

  // 重新发送起始信号
  if (!I2C_START(i2c_ctx)) {
    I2C_STOP(i2c_ctx);
    return 0;
  }
  // 发送设备地址(读)
  if (!I2C_SEND_ADDR(i2c_ctx, RADDR_24C02)) {
    I2C_STOP(i2c_ctx);
    return 0;
  }
  // 读取数据
  uint8_t data = I2C_READ_BYTE(i2c_ctx, false);
  // 发送结束信号
  I2C_STOP(i2c_ctx);

  return data;
}

static void write_byte_24c02(uint8_t addr, uint8_t data) {
  // 发送起始信号
  if (!I2C_START(i2c_ctx)) {
    return;
  }
  // 发送设备地址
  if (!I2C_SEND_ADDR(i2c_ctx, WADDR_24C02)) {
    I2C_STOP(i2c_ctx);
    return;
  }
  // 发送内部地址
  if (!I2C_SEND_BYTE(i2c_ctx, addr)) {
    I2C_STOP(i2c_ctx);
    return;
  }
  // 发送数据
  I2C_SEND_BYTE(i2c_ctx, data);
  // 发送结束信号
  I2C_STOP(i2c_ctx);

  // 等待
  DELAY_EEPROM;
}

void read_24c02(uint8_t addr, uint8_t *buff, uint8_t size) {
  // 发出 start 信号
  if (!I2C_START(i2c_ctx))
    return;

  // 发送写地址
  if (!I2C_SEND_ADDR(i2c_ctx, WADDR_24C02)) {
    return;
  }

  // 发送内部地址
  if (!I2C_SEND_BYTE(i2c_ctx, addr)) {
    return;
  }

  // 发送读地址
  if (!I2C_SEND_ADDR(i2c_ctx, RADDR_24C02)) {
    return;
  }

  for (uint8_t i = 0; i < size; i++) {
    buff[i] = I2C_SEND_BYTE(i2c_ctx, i < size - 1);
  }

  // 发送停止信号
  I2C_STOP(i2c_ctx);
}

void write_24c02(uint8_t addr, uint8_t *buff, uint8_t size) {
  // 发出 start 信号
  if (!I2C_START(i2c_ctx)) {
    return;
  }

  // 发送写地址
  if (!I2C_SEND_ADDR(i2c_ctx, WADDR_24C02)) {
    I2C_STOP(i2c_ctx);
    return;
  }

  // 发送内部地址
  if (!I2C_SEND_BYTE(i2c_ctx, addr)) {
    I2C_STOP(i2c_ctx);
    return;
  }

  for (uint8_t i = 0; i < size; i++) {
    if (!I2C_SEND_BYTE(i2c_ctx, buff[i])) {
      break;
    }
  }

  // 发送停止信号
  I2C_STOP(i2c_ctx);

  // 等待5ms
  DELAY_EEPROM;
}

eeprom_t eeprom_24c02 = {
    .init = init_24c02,
    .read_byte = read_byte_24c02,
    .write_byte = write_byte_24c02,
    .read = read_24c02,
    .write = write_24c02,
};
