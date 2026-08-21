#ifndef __BSP_SRAM_H__
#define __BSP_SRAM_H__

#include "type.h"

/**
 * SRAM_BASE 地址
 * 1. 使用 FSMC-BANK1来驱动 SRAM, BANK1起始地址 0x6000 0000
 * 2. BANK1 分为4块64M存储块, 使用存储块3(FSMC_NE3)作为片选
 * 存储块3起始地址: 0x6000 0000 + (3 - 1) << 26 = 0x68000000
 * 3. 使用 FSMC_A18 ~ A0 连接 SRAM地址, 地址范围: 0x00000 ~ 7FFFF
 * 5. 数据总线宽度为16bit, HADDR[25:1] -> ADDR[24:0]: 地址范围 0x00000 ~ 0xFFFFF
 * 因此 sram 地址范围: 0x68000000 ~ 0x680FFFFF
 */
#define ESRAM_BASE 0x68000000 // 基地址
#define ESRAM_SIZE 0x10000    // 64KB

void bsp_sram_init(void);

// 16bit读写
#define SRAM_WRITE16(addr, data)                                               \
  (*(volatile uint16_t *)(ESRAM_BASE + (addr)) = (data))
#define SRAM_READ16(addr) (*(volatile uint16_t *)(ESRAM_BASE + (addr)))

// 8bit读写 (8位访问需要BHE/BLE配合, 但FSMC会自动处理)
#define SRAM_WRITE8(addr, data)                                                \
  (*(volatile uint8_t *)(ESRAM_BASE + (addr)) = (data))
#define SRAM_READ8(addr) (*(volatile uint8_t *)(ESRAM_BASE + (addr)))

#endif
