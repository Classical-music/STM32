#include "eeprom.h"
#include "gpio.h"
#include "i2c/i2c.h"
#include "tim/tim.h"

/**
 * 24c02 相关参数
 * 1. I2C 引脚 PB6-SCL & PB7-SDA
 * 2. I2C1 未重映射
 * 3. 24c02 地址: 1010000 | 0/1 写/读
 * 4. 使用软件模拟I2C
 *
 * 2Kbit = 256KB = 32Page
 * 1Page = 8B
 *
 */

#define RADDR_24C02 0xA1
#define WADDR_24C02 0xA0

#define DELAY_EEPROM delay_ms(5)

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

static void read_24c02(uint8_t addr, uint8_t *buff, uint16_t size) {
  if (size + addr > 256) {       // 24C02支持的最大尺寸是 256
    size = 256 - (uint16_t)addr; // 防止读回绕
  }

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

  // 重新发送起始信号
  if (!I2C_START(i2c_ctx)) {
    printf("%s:%d i2c_start failed\n", __FILE__, __LINE__);
    I2C_STOP(i2c_ctx);
    return;
  }
  // 发送读地址
  if (!I2C_SEND_ADDR(i2c_ctx, RADDR_24C02)) {
    I2C_STOP(i2c_ctx);
    return;
  }
  // 读取数据
  for (uint16_t i = 0; i < size; i++) {
    buff[i] = I2C_READ_BYTE(i2c_ctx, i < size - 1);
  }

  // 发送停止信号
  I2C_STOP(i2c_ctx);
}

static void write_24c02_page(uint8_t addr, uint8_t *buff, uint16_t size) {
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

/**
 * 24C02 支持页写入:
 * 	 即每次连续写入一页, 若写到页尾部, 则从该页头部重新写入
 * 总大小为 256, 8位addr 大小足够
 * 每页大小为 8B, 因此低三位为页内地址
 */
static void write_24c02(uint8_t addr, uint8_t *buff, uint16_t size) {
  if (size + addr > 256) { // 24C02支持的最大尺寸是 256
    size = 256 - (uint16_t)addr;
  }
  uint8_t offset = addr & 0x7; // 页内地址
  uint8_t remain = 8 - offset; // 页内剩余大小
  printf("addr:%d size:%d\n", addr, size);

  while (1) {
    if (remain >= size) {
      // 页内剩余大小大于写入大小, 当前页空间足够
      return write_24c02_page(addr, buff, size); // 直接写入
    }
    // 否则 remain < size 当前页空间不足, 先写完当前页
    write_24c02_page(addr, buff, remain);
    addr += remain;
    buff += remain;
    size -= remain;
    remain = 8; // 下次循环 remain 剩余空间为 8
  }
}

eeprom_t eeprom_24c02 = {
    .init = init_24c02,
    .read_byte = read_byte_24c02,
    .write_byte = write_byte_24c02,
    .read = read_24c02,
    .write = write_24c02,
};
