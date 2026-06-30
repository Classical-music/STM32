#include "lcd.h"
#include "fsmc/fsmc.h"
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

static fsmc_ctx_t fsmc_ctx = {
    .bank = FSMC_Bank1,
    .bankE = FSMC_Bank1E,
    .neId = 4, // 使用 Bank1的NE4
    .mwid = fsmc_mwid_16,
};

void lcd_init(void) {
  // RCC LCD_CS, LCD_RS, LCD_RW, LCD_WR, LCD_BL
  RCC_GPIO_EN(RCC_GPIOB | RCC_GPIOD | RCC_GPIOG);
  // RCC PD
  RCC_GPIO_EN(RCC_GPIOD | RCC_GPIOE);

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

  FSMC_INIT(fsmc_ctx);

  timer_delay_ms(5); // 初始化FSMC后, 必须等待一段时间

  if (!lcddev.init())
    return;
  lcd_display_dir(LCD_DIR_VERT); // 默认设为竖屏

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
void lcd_write_ram_prepare(void) {
  LCD->reg = lcddev.wramcmd; //
}

// 设置LCD显示方向: 0-竖屏 1-横屏
void lcd_display_dir(lcd_dir_t dir) {
  if (lcddev.dir == dir)
    return;
  lcddev.dir = dir;
  SWAP(lcddev.width, lcddev.height);
  lcddev.scan_dir(dir);
}

// 画点
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color) {
  lcddev.set_cursor(x, y); // 设置光标位置
  lcd_write_ram_prepare(); // 开始写入 GRAM
  LCD->ram = color;
}

// 全屏填充颜色
void lcd_clear(uint16_t color) {
  // 计算像素点数
  uint32_t point = lcddev.width * lcddev.height;
  lcddev.set_cursor(0x00, 0x0000); // 设置光标位置

  lcd_write_ram_prepare(); // 开始写入 GRAM
  for (uint32_t i = 0; i < point; i++) {
    LCD->ram = color;
  }
}

// 填充实心圆
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {}
// 画圆
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {}
// 画水平线
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color) {
  if (len == 0 || x > lcddev.width || y > lcddev.height)
    return;
  lcd_fill(x, y, x + len - 1, y, color);
}
// 设置窗口
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height) {
  uint16_t ex = sx + width;
  uint16_t ey = sy + height;

  // 左上角-X
  lcd_write_regno(lcddev.setxcmd);
  lcd_write_data(sx >> 8);
  lcd_write_regno(lcddev.setxcmd + 1);
  lcd_write_data(sx & 0xFF);
  // 左上角-Y
  lcd_write_regno(lcddev.setycmd);
  lcd_write_data(sy >> 8);
  lcd_write_regno(lcddev.setycmd + 1);
  lcd_write_data(sy & 0xFF);

  // 右下角-X
  lcd_write_regno(lcddev.setxcmd);
  lcd_write_data(ex >> 8);
  lcd_write_regno(lcddev.setxcmd + 1);
  lcd_write_data(ex & 0xFF);
  // 右下角-Y
  lcd_write_regno(lcddev.setycmd);
  lcd_write_data(ey >> 8);
  lcd_write_regno(lcddev.setycmd + 1);
  lcd_write_data(ey & 0xFF);
}
// 纯色填充矩阵
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey,
              uint16_t color) {
  for (uint16_t i = sy; i <= ey; i++) {
    lcddev.set_cursor(sx, i); // 设置光标位置
    lcd_write_ram_prepare();  // 开始写入 GRAM
    for (uint16_t j = sx; j <= ex; j++) {
      LCD->ram = color; // 显示颜色
    }
  }
}
// 彩色填充矩阵
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey,
                    uint16_t *color) {}
// 画直线
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                   uint16_t color) {}
// 画矩形
void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                   uint16_t color) {}

// 显示一个字符
void lcd_show_char(uint16_t x, uint16_t y, char ch, uint8_t size, uint8_t mode,
                   uint16_t color) {}
// 显示数字
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len,
                  uint8_t size, uint16_t color) {}
// 扩展显示数字
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len,
                   uint8_t size, uint8_t mode, uint16_t color) {}
// 显示字符串
void lcd_show_str(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  uint8_t size, char *p, uint16_t color) {}
