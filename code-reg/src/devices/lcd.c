#include "lcd.h"
#include "rcc.h"
#include "timer.h"

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

void lcd_init(void) {
  // RCC LCD_CS, LCD_RS, LCD_RW, LCD_WR, LCD_BL
  RCC_GPIO_EN(RCC_GPIOB | RCC_GPIOD | RCC_GPIOG);
  // RCC PD
  RCC_GPIO_EN(RCC_GPIOD | RCC_GPIOE);
  RCC->AHBENR |= RCC_AHBENR_FSMCEN; // FSMC

  // LCD_CS - PG12 - 复用推挽输出
  GPIO_MODE_SET(GPIOG, 12, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  // LCD_RS - PG0 - 复用推挽输出
  GPIO_MODE_SET(GPIOG, 0, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  // LCD_RD - PD4 - 复用推挽输出
  GPIO_MODE_SET(GPIOD, 4, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  // LCD_WR - PD5 - 复用推挽输出
  GPIO_MODE_SET(GPIOD, 5, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  // LCD_BL - 通用推挽输出
  GPIO_MODE_SET(GPIOB, 0, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP);

  // LCD_D[15:0] IO - 复用推挽输出
  GPIO_MODE_SET(GPIOD, 0, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOD, 1, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOD, 8, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOD, 9, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOD, 10, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOD, 14, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOD, 15, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 7, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 8, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 9, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 10, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 11, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 12, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 13, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 14, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  GPIO_MODE_SET(GPIOE, 15, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);

  // FSMC 时钟来自HCLK, 72MHz
  // 寄存器清零
  // bank1有NE1~4,每一个有一个 BCR + TCR, 共8个寄存器
  // 使用NE4
  FSMC_BCR = 0x0; // 寄存器清零
  FSMC_BTR = 0x0;
  FSMC_BWTR = 0x0;

  // 时序配置 - 模式A
  // BCR片选控制 配置
  FSMC_BCR |= FSMC_BCR4_EXTMOD; // 扩展模式使能, 允许读写时序不同
  FSMC_BCR |= FSMC_BCR4_WREN;   // 写使能
  FSMC_BCR |= FSMC_BCR4_MWID_0; // 数据总线宽度 01-16bit
  FSMC_BCR &= ~FSMC_BCR4_MTYP;  // 存储器类型 00-SRAM (伪装)

  // BTR读片选时序 配置
  FSMC_BTR &= ~FSMC_BTR4_ACCMOD; // 00-访问模式A
  FSMC_BTR |= 15 << 8;           // 数据保持时间 （1/72M)*15=208ns
  FSMC_BTR |= 1 & 0xF;           // 地址建立时间 (因F1有性能问题,设置1够了)

  // BWTR写片选时序 配置(EXTMOD=1)
  FSMC_BWTR &= ~FSMC_BWTR4_ACCMOD; // 00-访问模式A
  FSMC_BWTR |= 3 << 8;             // 数据保持时间
  FSMC_BWTR |= 1 & 0xF;            // 地址建立时间 (因F1有性能问题,设置1够了)

  FSMC_BCR |= FSMC_BCR4_MBKEN; // 使能Bank1

  timer_delay_ms(5); // 初始化FSMC后, 必须等待一段时间

  if (!lcddev.init())
    return;
  lcd_display_dir(0); // 默认设为竖屏

  // 点亮屏幕
  LCD_BL(1);

  lcd_clear(0xF800);
}

// LCD 读写
void lcd_write_data(volatile uint16_t data) {
  LCD->ram = data; // 写数据
}
void lcd_write_regno(volatile uint16_t regno) {
  LCD->reg = regno; // 写寄存器ID
}
void lcd_write_reg(uint16_t regno, uint16_t data) {
  LCD->reg = regno; // 选择寄存器ID
  LCD->ram = data;  // 写入数据
}
uint16_t lcd_read_data(void) {
  volatile uint16_t data;
  data = LCD->ram;
  return data;
}
// 设置LCD显示方向: 0-竖屏 1-横屏
void lcd_display_dir(uint8_t dir) {
  if (lcddev.dir == dir)
    return;
  lcddev.dir = dir;
  SWAP(lcddev.width, lcddev.height);
  lcddev.scan_dir(L2R_T2B);
}

void lcd_clear(uint16_t color) {
  uint32_t index = 0;
  uint32_t totalpoint = lcddev.width;
  totalpoint *= lcddev.height;     /* 得到总点数 */
  lcddev.set_cursor(0x00, 0x0000); // 设置光标位置
  LCD->reg = lcddev.wramcmd;       // 开始写入GRAM

  for (index = 0; index < totalpoint; index++) {
    LCD->ram = color;
  }
}
