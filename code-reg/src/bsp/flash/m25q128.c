#include "flash.h"
#include "gpio.h"
#include "spi/spi.h"

// 片选
#define CS(en)                                                                 \
  do {                                                                         \
    if (en) {                                                                  \
      GPIO_LOW(GPIOB, 12);                                                     \
    } else {                                                                   \
      GPIO_HIGH(GPIOB, 12);                                                    \
    }                                                                          \
  } while (0)

// flash 命令
// 读取ID
#define FLASH_ReleasePowerDown 0xAB
#define FLASH_JedecDeviceID 0x9F

// 读写 SR
#define FLASH_ReadSR1 0x05
#define FLASH_ReadSR2 0x35
#define FLASH_ReadSR3 0x15

#define FLASH_WriteSR1 0x01
#define FLASH_WriteSR2 0x31
#define FLASH_WriteSR3 0x11

// 写使能
#define FLASH_WriteEnable 0x06
// 扇区擦除
#define FLASH_SectorErase 0x20
// 页写入
#define FLASH_PageProgram 0x02

#define FLASH_ReadData 0x03
#define FLASH_PowerDown 0xB9
#define FLASH_Enable4ByteAddr 0xB7
#define FLASH_Exit4ByteAddr 0xE9
#define FLASH_SetReadParam 0xC0
#define FLASH_EnterQPIMode 0x38
#define FLASH_ExitQPIMode 0xFF

// SR 位
#define FLASH_SR1_WIP 0x01

// 前向声明
static void read_m25q128(uint32_t addr, uint8_t *buff, uint16_t size);

// 读取芯片 JedecID
static bool flash_check_id(void) {
  // 先唤醒
  CS(true);
  spi_rw(FLASH_ReleasePowerDown);
  CS(false);
  for (uint8_t i = 0xF; i > 0; i++)
    ;

  // 再读取 JedecID
  uint32_t id = 0;
  CS(true);
  spi_rw(FLASH_JedecDeviceID);
  id = spi_rw(0x00) << 16; // MenuID
  id |= spi_rw(0x00) << 8; // Memory Type
  id |= spi_rw(0x00);      // Memory Density
  CS(false);
  // printf("JedecID: %x\n", id);

  return id == 0x522118;
}

// 读 SR
static uint8_t flash_read_sr(uint8_t cmd) {
  CS(true);
  spi_rw(cmd); // 发送读寄存器命令
  uint8_t byte = spi_rw(0xFF);
  CS(false);

  return byte;
}

// 等待 flash 忙: 根据SR1-WIP位
static void flash_wait_busy(void) {
  while (flash_read_sr(FLASH_ReadSR1) & FLASH_SR1_WIP)
    ; // 等待 WIP 位清空
}

// 写使能(页写入&擦除时需要)
static void flash_write_enable(void) {
  CS(true);
  spi_rw(FLASH_WriteEnable);
  CS(false);
}

// 写入三字节页地址 (读取&页写入&擦除时需要)
static void flash_send_addr(uint32_t addr) {
  spi_rw(addr >> 16); // bit23 ~ bit16
  spi_rw(addr >> 8);  // bit15 ~ bit8
  spi_rw(addr);       // bit7 ~ bit0
}

// 擦除一个扇区, 扇区大小为 4KB
static void flash_erase_sector(uint32_t saddr) {
  saddr &= (~0xFFF);
  flash_write_enable(); // 写使能
  flash_wait_busy();    // 等待空闲

  CS(true);
  spi_rw(FLASH_SectorErase); // 发送写页命令
  flash_send_addr(saddr);    // 发送地址
  CS(false);

  flash_wait_busy(); // 等待扇区擦除完成
}

// 页内写入, 最大可写入一页
static void flash_write_page(uint32_t addr, uint8_t *buff, uint16_t size) {
  flash_write_enable(); // 写使能

  CS(true);
  spi_rw(FLASH_PageProgram); // 发送写页命令
  flash_send_addr(addr);     // 发送地址

  // 写入数据
  for (uint16_t i = 0; i < size; i++) {
    spi_rw(buff[i]); // 循环写入
  }
  CS(false);

  flash_wait_busy(); // 等待写入结束
}

// 此时已擦除, 无需校验该页数据
// 页大小为256B，因此页内地址为低8位
static void flash_write_sector_nocheck(uint32_t addr, uint8_t *buff,
                                       uint16_t size) {
  uint16_t offset = addr & 0xFF;  // 页内地址
  uint16_t remain = 256 - offset; // 页内剩余

  while (1) {
    if (remain >= size) {
      // 页内剩余大小大于写入大小, 当前页空间足够
      return flash_write_page(addr, buff, size); // 直接写入
    }
    // 否则 remain < size 当前页空间不足, 先写完当前页
    flash_write_page(addr, buff, remain);
    addr += remain;
    buff += remain;
    size -= remain;
    remain = 256; // 下次循环, remain 剩余空间为 256
  }
}

// 扇区大小 4KB, 扇区内地址 12位
static uint8_t g_flash_buf[4096]; // 扇区缓存
static void flash_write_sector(uint32_t addr, uint8_t *buff, uint16_t size) {
  uint32_t saddr = addr & (~0xFFF); // 计算扇区首地址
  uint16_t offset = addr & 0xFFF;   // 扇区内偏移
  uint16_t remain = 4096 - offset;  // 扇区内剩余空间

  read_m25q128(saddr, g_flash_buf, 4096); // 读出整个扇区
  uint16_t i = offset;
  for (; i < 4096; i++) { // 校验数据
    if (g_flash_buf[i] != 0xFF) {
      break; // 需要擦除, 直接退出for循环
    }
  }

  if (i < 4096) {
    // 需要擦除
    flash_erase_sector(saddr); // 擦除这个扇区
    for (uint16_t j = 0; j < remain; j++) {
      g_flash_buf[offset + j] = buff[j]; // 拷贝写入数据
    }

    flash_write_sector_nocheck(saddr, g_flash_buf, 4096); // 写入整个扇区
  } else {
    // 无需擦除, 直接写入
    flash_write_sector_nocheck(addr, buff, remain); // 直接写扇区
  }
}

/**
 * m25q128 相关参数
 * 1.SPI 引脚:
 * 	PB12 - CS 片选 - 通用推挽输出
 * 	PB13 - SCK 时钟 - 复用推挽输出
 *  PB14 - MISO 输出 - 复用推挽输出
 *  PB15 - MOSI 输入 - 复用推挽输出
 * 2.SPI2 未重映射
 */

static void init_m25q128(void) {
  // CS 初始化
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
  GPIO_MODE_SET(GPIOB, 12, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP);
  CS(false);

  // RCC
  RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
  // 引脚模式
  GPIO_MODE_SET(GPIOB, 13, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP); // SCK
  GPIO_MODE_SET(GPIOB, 14, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOB, 15, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);

  spi_init();

  if (!flash_check_id())
    return;
}

/**
 * 读取 SPI FLASH
 */
static void read_m25q128(uint32_t addr, uint8_t *buff, uint16_t size) {
  uint16_t i;

  CS(true);
  spi_rw(FLASH_ReadData); // 发送读取命令
  flash_send_addr(addr);  // 发送地址

  for (i = 0; i < size; i++) {
    buff[i] = spi_rw(0xFF); // 循环读取
  }

  CS(false);
}

/**
 * 写 SPI Flash
 * 1Block = 16Sector(扇区)
 * 1Sector=4KB
 * 1Page=256B
 * 擦除的最小单位是 Sector
 */
static void write_m25q128(uint32_t addr, uint8_t *buff, uint16_t size) {
  uint16_t offset = addr & 0xFFF;  // 扇区内偏移
  uint16_t remain = 4096 - offset; // 扇区剩余空间

  while (1) {
    if (remain > size) {
      // 扇区内剩余大小大于写入大小, 当前扇区空间足够
      return flash_write_sector(addr, buff, size);
    }
    // 否则 remain < size 当前扇区控件不足, 先写完当前扇区
    flash_write_sector(addr, buff, remain);
    addr += remain;
    buff += remain;
    size -= remain;
    remain = 4096; // 下次循环, remain 剩余空间
  }
}

flash_t flash_m25q128 = {
    .init = init_m25q128,
    .write = write_m25q128,
    .read = read_m25q128,
};
