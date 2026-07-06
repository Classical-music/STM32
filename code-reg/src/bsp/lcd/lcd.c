#include "lcd.h"
#include "fsmc/fsmc.h"
#include "rcc.h"
#include "tim.h"

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

  delay_ms(5); // 初始化FSMC后, 必须等待一段时间

  if (!lcddev.init())
    return;
  lcd_set_dir(LCD_DIR_VERT); // 默认设为竖屏

  // 点亮屏幕
  LCD_BL(1);

  LCD_CLEAR(0xFFFFFF);
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
  volatile uint16_t ram;
  ram = LCD->ram;
  return ram;
}

// 开启 LCD
void lcd_on(void) {
  lcd_write_regno(lcddev.disponCmd); //
}
// 关闭 LCD
void lcd_off(void) {
  lcd_write_regno(lcddev.dispoffCmd); //
}
// 设置LCD显示方向: 0-竖屏 1-横屏
void lcd_set_dir(lcd_dir_t dir) {
  if (lcddev.dir == dir)
    return;
  lcddev.dir = dir;
  SWAP(lcddev.width, lcddev.height);
  lcddev.scan_dir(dir);
}

static void lcd_write_ram_prepare(void) {
  LCD->reg = lcddev.wramcmd; //
}

// ==================== UI 绘制 ====================

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

// 画点
static void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color) {
  lcddev.set_cursor(x, y); // 设置光标位置
  lcd_write_ram_prepare(); // 开始写入 GRAM
  LCD->ram = color;
}

// 纯色填充矩阵
void lcd_fill(point_t *p1, point_t *p2, uint16_t color) {
  for (uint16_t i = p1->y; i <= p2->y; i++) {
    lcddev.set_cursor(p1->x, i); // 设置光标位置
    lcd_write_ram_prepare();     // 开始写入 GRAM
    for (uint16_t j = p1->x; j <= p2->x; j++) {
      LCD->ram = color; // 显示颜色
    }
  }
}
// 彩色填充矩阵
void lcd_color_fill(point_t *p1, point_t *p2, uint16_t *color) {}
// 填充圆形
void lcd_fill_circle(point_t *p, uint16_t r, uint16_t color) {}

// 画水平线
void lcd_draw_hline(point_t *p, uint16_t len, uint16_t color) {
  if (len == 0 || p->x > lcddev.width || p->y > lcddev.height)
    return;
  point_t p2 = {p->x + len - 1, p->y};
  lcd_fill(p, &p2, color);
}
// 画直线 Bresenham
void lcd_draw_line(point_t *p1, point_t *p2, uint16_t color) {
  int dx = p2->x - p1->x; // 坐标增量
  int dy = p2->y - p1->y;

  int incx = dx > 0 ? 1 : -1; // 步进方向
  int incy = dy > 0 ? 1 : -1;

  int err = dx - dy; // 初始误差项

  uint16_t x = p1->x;
  uint16_t y = p1->y;

  while (1) {
    lcd_draw_point(x, y, color); // 绘制当前点

    if (x == p2->x && y == p2->y)
      break; // 到达终点退出

    int e2 = err << 2; // 用2*err判断符号, 避免浮点

    if (e2 > -dy) { // 修正x方向
      err -= dy;
      x += incx;
    }
    if (e2 < dx) { // 修正 y 方向
      err += dx;
      y += incy;
    }
  }
}
// 画矩形
void lcd_draw_rect(rect_t *rect, uint16_t color) {
  uint16_t x1 = rect->x, x2 = rect->x + rect->width;
  uint16_t y1 = rect->y, y2 = rect->y + rect->height;
  point_t p1 = {x1, y1}, p2 = {x2, y1};
  point_t p3 = {x1, y2}, p4 = {x2, y2};
  lcd_draw_line(&p1, &p2, color);
  lcd_draw_line(&p2, &p4, color);
  lcd_draw_line(&p1, &p3, color);
  lcd_draw_line(&p3, &p4, color);
}
// 画圆 Bresenham 算法
void lcd_draw_circle(point_t *p, uint8_t r, uint16_t color) {
  int x = 0;
  int y = r;
  int pa = 1 - r; // 初始决策参数

  while (x <= y) {
    lcd_draw_point(p->x + x, p->y + y, color); // 第一象限
    lcd_draw_point(p->x - x, p->y + y, color); // 第二象限
    lcd_draw_point(p->x - x, p->y - y, color); // 第三象限
    lcd_draw_point(p->x + x, p->y - y, color); // 第四象限
    lcd_draw_point(p->x + y, p->y + x, color); // 对称翻转
    lcd_draw_point(p->x - y, p->y + x, color); //
    lcd_draw_point(p->x - y, p->y - x, color); //
    lcd_draw_point(p->x + y, p->y - x, color); //

    // 根据决策参数更新
    if (pa < 0) {
      pa += (x << 2) + 3; // 取正右方点
    } else {
      pa += ((x - y) << 2) + 5; // 取右下方点
      y--;
    }
    x++;
  }
}

// 清屏
void lcd_clear(uint16_t color);
// 设置窗口
void lcd_set_window(rect_t *rect) {
  uint16_t ex = rect->x + rect->width;
  uint16_t ey = rect->y + rect->height;

  // 左上角-X
  lcd_write_regno(lcddev.setxcmd);
  lcd_write_data(rect->x >> 8);
  lcd_write_regno(lcddev.setxcmd + 1);
  lcd_write_data(rect->x & 0xFF);
  // 左上角-Y
  lcd_write_regno(lcddev.setycmd);
  lcd_write_data(rect->y >> 8);
  lcd_write_regno(lcddev.setycmd + 1);
  lcd_write_data(rect->y & 0xFF);

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

// 显示一个字符
static const uint8_t charSet[][32] = {
    {
        0x00,
    },
};
void lcd_show_char(point_t *p, char ch, uint16_t color) {
  for (uint8_t i = 0; i < 16; i++) { // 行扫描
    uint8_t rowId = i << 1;
    uint16_t tmp =
        (charSet[ch][rowId] << 8) | charSet[ch][rowId + 1]; // 取出这一行的2B
    for (uint8_t j = 0; j < 16; j++) {
      if (tmp & 0x8000) {                          // 先画高位的点
        lcd_draw_point(p->x + j, p->y + i, color); // 1就点亮
      }
      tmp <<= 1;
    }
  }
}
//  显示数字
void lcd_show_num(point_t *p, uint32_t num, uint16_t color) {}
// 扩展显示数字
void lcd_show_xnum(point_t *p, uint32_t num, uint16_t color) {}
//  显示字符串
void lcd_show_str(rect_t *rect, char *str, uint16_t color) {
  point_t p = {rect->x, rect->y};
  while (*str) {
    lcd_show_char(&p, *str++, color);
    p.x += 16;
  }
}
