#ifndef __LCD_H__
#define __LCD_H__

#include "gpio.h"
#include "type.h"

/**
 * GPIO 引脚:
 * LCD_CS - FSMC_NE4 - PG12 片选
 * LCD_RS - FSMC_A10 - PG0 数据/命令
 *
 * LCD_RD - FSMC_NOE - PD4 读使能
 * LCD_WR - FSMC_NWE - PD5 写使能
 *
 * LCD_BL - PB0
 *
 * D[15:0] ~ FSMC_Dx[15:0]
 * D0:1 - PD14:15
 * D2:3 - PD0:1
 * D4:12 - PE7:15
 * D13:15 - PD8:10
 */

// LCD_BL 背光 ========================================
#define LCD_BL(x)                                                              \
  do {                                                                         \
    if (x)                                                                     \
      GPIO_HIGH(GPIOB, 0);                                                     \
    else                                                                       \
      GPIO_LOW(GPIOB, 0);                                                      \
  } while (0)

// LCD 地址结构体
typedef struct {
  volatile uint16_t reg;
  volatile uint16_t ram;
} lcd_t;

typedef enum {
  L2R_T2B = 0, // 左右, 上下
  L2R_B2T,     // 左右, 下上
  R2L_T2B,     // 右左, 上下
  R2L_B2T,     // 右左, 下上

  T2B_L2R = 4, // 上下, 左右
  T2B_R2L,     // 上下, 右左
  B2T_L2R,     // 下上, 左右
  B2T_R2L,     // 下上, 右左
} lcd_scan_dir_t;

// LCD 重要参数
typedef struct {
  bool (*init)(void);
  void (*scan_dir)(lcd_scan_dir_t);
  void (*set_cursor)(uint16_t x, uint16_t y);
  uint16_t width;
  uint16_t height;
  uint8_t dir;
  uint16_t wramcmd; // 开始写 gram 指令
  uint16_t setxcmd; // 设置x坐标指令
  uint16_t setycmd; // 设置y坐标指令
} lcd_dev_t;

extern lcd_dev_t lcddev;

/**
 * LCD_BASE 地址
 * 1. 使用 FSMC-BANK1来驱动 TFTLCD, BANK1起始地址 0x6000 0000
 * 2. BANK1 分为4块64M存储块, 使用存储块4(FSMC_NE4)作为片选
 * 存储块4起始地址: 0x6000 0000 + (4 - 1) << 26 = 0x6C000000
 * 3. 使用 FSMC_A10 连接 LCD_RS, 对应地址 2^10 = 0x400
 * 5. 数据总线宽度为16bit, HADDR[25:1] -> ADDR[24:0], 对应地址 0x800
 * 由此 ram 地址为 0x6C00 0800, reg地址为 0x6C0007FE
 */
#define LCD_BASE 0x6C0007FE
#define LCD ((lcd_t *)LCD_BASE)

// LCD_FSMC寄存器地址 ========================================
#define FSMC_BCR FSMC_Bank1->BTCR[6]   // ne4
#define FSMC_BTR FSMC_Bank1->BTCR[7]   // ne4
#define FSMC_BWTR FSMC_Bank1E->BWTR[6] // ne4

void lcd_init(void);

void lcd_write_data(volatile uint16_t data);
void lcd_write_regno(volatile uint16_t regno);
void lcd_write_reg(uint16_t regno, uint16_t data);
uint16_t lcd_read_data(void);
void lcd_display_dir(uint8_t dir);
void lcd_clear(uint16_t color);
#endif
