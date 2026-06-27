#include "flash.h"
#include "gpio.h"
#include "spi/spi.h"
#include "timer.h"

#define CS(en)                                                                 \
  do {                                                                         \
    if (en) {                                                                  \
      GPIO_LOW(GPIOB, 12);                                                     \
    } else {                                                                   \
      GPIO_HIGH(GPIOB, 12);                                                    \
    }                                                                          \
  } while (0)

// flash 命令
#define FLASH_WriteEnable 0x06
#define FLASH_WriteDisable 0x04
#define FLASH_ReadStatusReg1 0x05
#define FLASH_ReadStatusReg2 0x35
#define FLASH_ReadStatusReg3 0x15
#define FLASH_WriteStatusReg1 0x01
#define FLASH_WriteStatusReg2 0x31
#define FLASH_WriteStatusReg3 0x11
#define FLASH_ReadData 0x03
#define FLASH_FastReadData 0x0B
#define FLASH_FastReadDual 0x3B
#define FLASH_FastReadQuad 0xEB
#define FLASH_PageProgram 0x02
#define FLASH_PageProgramQuad 0x32
#define FLASH_BlockErase 0xD8
#define FLASH_SectorErase 0x20
#define FLASH_ChipErase 0xC7
#define FLASH_PowerDown 0xB9
#define FLASH_ReleasePowerDown 0xAB
#define FLASH_DeviceID 0xAB
#define FLASH_ManufactDeviceID 0x90
#define FLASH_JedecDeviceID 0x9F
#define FLASH_Enable4ByteAddr 0xB7
#define FLASH_Exit4ByteAddr 0xE9
#define FLASH_SetReadParam 0xC0
#define FLASH_EnterQPIMode 0x38
#define FLASH_ExitQPIMode 0xFF

// 前向声明
static uint16_t flash_read_id(void);
static uint8_t flash_read_sr(uint8_t regno);
static void flash_write_sr(uint8_t regno, uint8_t sr);
static void flash_write_enable(void);

/**
 * w25q128 相关参数
 * 1.SPI 引脚:
 * 	PB12 - CS 片选 - 通用推挽输出
 * 	PB13 - SCK 时钟 - 复用推挽输出
 *  PB14 - MISO 输出 - 复用推挽输出
 *  PB15 - MOSI 输入 - 复用推挽输出
 * 2.SPI2 未重映射
 */

static void init_w25q128(void) {
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

  uint16_t id = flash_read_id();
  printf("flashId: 0x%x\n", id);
  if (id != 0x5217) {     // 第一次读取, 可能有问题
    id = flash_read_id(); // 第二次读取
    printf("flashId: 0x%x\n", id);
    if (id != 0x5217) {
      return;
    }
  }

  uint8_t sr3 = flash_read_sr(3);
  if (!(sr3 & 0x01)) { // 不是4B地址模式
    // 设置4B地址模式
    flash_write_enable();   // 写使能
    sr3 |= 1 << 1;          // ADP=1, 上电4B地址模式
    flash_write_sr(3, sr3); // 写 SR3

    CS(true);
    spi_rw(FLASH_Enable4ByteAddr); // 使能4B地址指令
    CS(false);
  }
}

static uint16_t flash_read_id(void) {
  // 读取芯片ID
  CS(true);

  spi_rw(FLASH_ManufactDeviceID); // 发送读ID命令
  spi_rw(0);
  spi_rw(0);
  spi_rw(0);
  uint16_t devId = spi_rw(0xFF) << 8; // 高8位
  devId |= spi_rw(0xFF);              // 低8位

  CS(false);

  return devId;
}

static uint8_t flash_read_sr(uint8_t regno) {
  uint8_t command = FLASH_ReadStatusReg1;
  switch (regno) {
  case 1:
    command = FLASH_ReadStatusReg1; /* 读状态寄存器1指令 */
    break;

  case 2:
    command = FLASH_ReadStatusReg2; /* 读状态寄存器2指令 */
    break;

  case 3:
    command = FLASH_ReadStatusReg3; /* 读状态寄存器3指令 */
    break;

  default:
    command = FLASH_ReadStatusReg1;
    break;
  }

  CS(true);

  spi_rw(command); // 发送读寄存器命令
  uint8_t byte = spi_rw(0xFF);

  CS(false);
  printf("sr%d = %x\n", regno, byte);

  return byte;
}

static void flash_write_sr(uint8_t regno, uint8_t sr) {
  uint8_t command = FLASH_ReadStatusReg1;
  switch (regno) {
  case 1:
    command = FLASH_WriteStatusReg1; // 读SR1指令
    break;
  case 2:
    command = FLASH_WriteStatusReg2; // 读SR2指令
    break;
  case 3:
    command = FLASH_WriteStatusReg3; // 读SR3指令
    break;

  default:
    break;
  }

  CS(true);

  spi_rw(command); // 发送写寄存器命令
  spi_rw(sr);

  CS(false);
}

static void flash_write_enable(void) {
  CS(true);
  spi_rw(FLASH_WriteEnable);
  CS(false);
}

static void flash_send_addr(uint32_t addr) {
  spi_rw(addr >> 24); // bit31 ~ bit24
  spi_rw(addr >> 16); // bit23 ~ bit16
  spi_rw(addr >> 8);  // bit15 ~ bit8
  spi_rw(addr);       // bit7 ~ bit0
}

static void flash_wait_busy(void) {
  while ((flash_read_sr(1) & 0x01) == 0x01)
    ; // 等待 busy 位清空
}

void flash_write_page(uint32_t addr, uint8_t *buff, uint16_t size) {
  flash_write_enable(); // 写使能

  CS(true);
  spi_rw(FLASH_PageProgram); // 发送写页命令
  flash_send_addr(addr);     // 发送地址

  for (uint16_t i = 0; i < size; i++) {
    spi_rw(buff[i]); // 循环写入
  }

  CS(false);

  flash_wait_busy(); // 等待写入结束
}

static void flash_write_nocheck(uint32_t addr, uint8_t *buff, uint16_t size) {
  uint16_t pageRemain = 256 - addr % 256; // 单页剩余字节数
  if (size < pageRemain) {
    pageRemain = size;
  }

  while (1) {
    /* 当写入字节比页内剩余地址还少的时候, 一次性写完
     * 当写入直接比页内剩余地址还多的时候, 先写完整个页内剩余地址,
     * 然后根据剩余长度进行不同处理
     */
    flash_write_page(addr, buff, pageRemain);
    if (size == pageRemain) {
      break; // 写入结束了
    }

    buff += pageRemain;
    addr += pageRemain;
    size -= pageRemain;

    if (size > 256) {
      pageRemain = 256; // 剩余数据还大于一页, 一次写入一页
    } else {
      pageRemain = size; // 不够一页
    }
  }
}

// 擦除一个扇区
void flash_erase_sector(uint32_t saddr) {
  printf("erase_sec\n");
  saddr *= 4096;
  flash_write_enable(); // 写使能
  flash_wait_busy();    // 等待空闲

  CS(true);
  spi_rw(FLASH_SectorErase); // 发送写页命令
  flash_send_addr(saddr);    // 发送地址
  CS(false);

  flash_wait_busy(); // 等待扇区擦除完成
}

/**
 * 读取 SPI FLASH
 */
void read_w25q128(uint32_t addr, uint8_t *buff, uint16_t size) {
  uint16_t i;

  CS(true);
  spi_rw(FLASH_ReadData); // 发送读取命令
  flash_send_addr(addr);  // 发送地址

  for (i = 0; i < size; i++) {
    buff[i] = spi_rw(0xFF); // 循环读取
    printf("%x ", buff[i]);
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
static uint8_t g_flash_buf[4096]; // 扇区缓存
static void write_w25q128(uint32_t addr, uint8_t *buff, uint16_t size) {
  uint32_t secPos = addr / 4096;      // 扇区地址
  uint16_t secOff = addr % 4096;      // 扇区内偏移
  uint16_t secRemain = 4096 - secOff; // 扇区剩余空间

  if (size <= secRemain) {
    secRemain = size; // 剩余空间够用
  }

  while (1) {
    read_w25q128(secPos * 4096, g_flash_buf, 4096); // 读出整个扇区
    uint16_t i = 0;
    for (; i < secRemain; i++) { // 校验数据
      if (g_flash_buf[secOff + i] != 0xFF) {
        break; // 需要擦除, 直接退出for循环
      }
    }

    if (i < secRemain) {
      // 需要擦除
      flash_erase_sector(secPos); // 擦除这个扇区
      for (uint16_t j = 0; j < secRemain; j++) {
        g_flash_buf[secOff + j] = buff[j]; // 拷贝写入数据
      }

      flash_write_nocheck(secPos * 4096, g_flash_buf, 4096); // 写入整个扇区
    } else {
      // 无需擦除, 直接写入
      flash_write_nocheck(addr, buff, secRemain); // 直接写扇区
    }

    if (size == secRemain) {
      break; // 写入结束了
    }

    secPos++;   // 扇区地址增1
    secOff = 0; // 扇区偏移置零
    buff += secRemain;
    addr += secRemain;
    size -= secRemain;

    if (size > 4096) {
      secRemain = 4096; // 下个扇区写不完
    } else {
      secRemain = size; // 下个扇区可以写完
    }
  }
}

flash_t flash_w25q128 = {
    .init = init_w25q128,
    .write = write_w25q128,
    .read = read_w25q128,
};
