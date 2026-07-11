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

// 扫描方向
typedef enum {
  LCD_DIR_VERT, // 竖屏
  LCD_DIR_HOR,  // 横屏
} lcd_dir_t;

// LCD 重要参数
typedef struct {
  bool (*init)(void);
  void (*scan_dir)(lcd_dir_t);
  void (*set_cursor)(uint16_t x, uint16_t y);
  uint16_t width;
  uint16_t height;
  uint8_t dir;
  uint16_t disponCmd;  // 开启LCD
  uint16_t dispoffCmd; // 关闭LCD
  uint16_t wramcmd;    // 开始写 gram 指令
  uint16_t setxcmd;    // 设置x坐标指令
  uint16_t setycmd;    // 设置y坐标指令
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

// 初始化 LCD
void lcd_init(void);
// 基于 FSMC 的写 LCD 基础函数
void lcd_write_data(volatile uint16_t data);
void lcd_write_regno(volatile uint16_t regno);
void lcd_write_reg(uint16_t regno, uint16_t data);
uint16_t lcd_read_data(void);

void lcd_on(void);  // 开启 LCD
void lcd_off(void); // 关闭 LCD

void lcd_set_dir(lcd_dir_t dir); // 设置屏幕方向 横屏or竖屏

// ==================== UI 绘制 ====================
typedef struct {
  uint16_t x;
  uint16_t y;
} point_t;

typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
} rect_t;

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color); // 画点

void lcd_draw_hline(point_t *p, uint16_t len, uint16_t color); // 画水平线
void lcd_draw_line(point_t *p1, point_t *p2, uint16_t color);  // 画直线
void lcd_draw_rect(rect_t *rect, uint16_t color);              // 画矩形
void lcd_draw_circle(point_t *p, uint8_t r, uint16_t color);   // 画圆

void lcd_fill(point_t *p1, point_t *p2, uint16_t color);        // 纯色填充矩阵
void lcd_color_fill(point_t *p1, point_t *p2, uint16_t *color); // 彩色填充矩阵
void lcd_fill_circle(point_t *p, uint16_t r, uint16_t color);   // 填充圆形

void lcd_clear(uint16_t color);    // 清屏
void lcd_set_window(rect_t *rect); // 设置窗口

void lcd_show_char(point_t *p, char ch, uint16_t color);      // 显示一个字符
void lcd_show_num(point_t *p, uint32_t num, uint16_t color);  // 显示数字
void lcd_show_xnum(point_t *p, uint32_t num, uint16_t color); // 扩展显示数字
void lcd_show_str(rect_t *rect, char *str, uint16_t color);   // 显示字符串

// 将 RGB888 -> RGB565
#define RGB565_R(color) ((color >> 8) & 0xF800)
#define RGB565_G(color) ((color >> 5) & 0x07E0)
#define RGB565_B(color) ((color >> 3) & 0x001F)
#define RGB565(color)                                                          \
  (uint16_t)(RGB565_R(color) | RGB565_G(color) | RGB565_B(color))

#define LCD_DRAW_HLINE(p, len, color) lcd_draw_hline(&p, len, RGB565(color))
#define LCD_DRAW_LINE(p1, p2, color) lcd_draw_line(&p1, &p2, RGB565(color))
#define LCD_DRAW_RECT(rect, color) lcd_draw_rect(&rect, RGB565(color))
#define LCD_DRAW_CIRCLE(p, r, color) lcd_draw_circle(&p, r, RGB565(color))

#define LCD_CLEAR(color) lcd_clear(RGB565(color))

#endif
