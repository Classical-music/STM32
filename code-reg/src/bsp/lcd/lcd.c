#include "lcd.h"
#include "fsmc/fsmc.h"
#include "rcc.h"
#include "tim/tim.h"

/* LCD 使用 DMA1 Channel1 发送 RGB565 像素数据。 */
static dma_t lcd_dma = {
    .config = {
        .controller = DMA_CONTROLLER_1,
        .channel = DMA_CHANNEL_1,
        .direction = DMA_DIR_MEMORY_TO_MEMORY,
        .source_width = DMA_WIDTH_16BIT,
        .destination_width = DMA_WIDTH_16BIT,
        .source_increment = true,
        .destination_increment = false,
        .circular = false,
        .priority = 2,
        .interrupt_priority = IR_PRI_0,
    },
};

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
  GPIO_SET_MODES(GPIOD, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 0, 1, 8, 9, 10, 14,
                 15);
  GPIO_SET_MODES(GPIOE, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP, 7, 8, 9, 10, 11, 12,
                 13, 14, 15);

  delay_ms(1000);

  fsmc_ne4_init();

  delay_ms(1000);

  if (dma_init(&lcd_dma) != DMA_OK)
    return;

  delay_ms(1000); // 初始化FSMC后, 必须等待一段时间

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

  switch (dir) {
  case LCD_DIR_VERT: // 竖屏
    lcddev.width = 480;
    lcddev.height = 800;
    break;
  case LCD_DIR_HOR: // 横屏
    lcddev.width = 800;
    lcddev.height = 480;
    break;

  default:
    return;
  }
  lcddev.dir = dir;
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
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color) {
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
void lcd_color_fill(point_t *p1, point_t *p2, const uint16_t *color) {
  if (p1->x > p2->x || p1->y > p2->y || p2->x >= lcddev.width ||
      p2->y >= lcddev.height)
    return;

  rect_t rect = {
      .x = p1->x,
      .y = p1->y,
      .width = p2->x - p1->x + 1,
      .height = p2->y - p1->y + 1,
  };
  uint32_t point = (uint32_t)rect.width * rect.height;

  lcd_set_window(&rect);
  lcd_write_ram_prepare();
  while (point--)
    LCD->ram = *color++;
}

/* DMA 出错时复用完成回调，确保上层不会永久等待。 */
static void lcd_dma_error_callback(void *user_arg) {
  if (lcd_dma.complete_callback != NULL)
    lcd_dma.complete_callback(user_arg);
}

/* 使用 DMA 将连续 RGB565 数据写入 LCD 的固定 RAM 地址。 */
dma_result_t lcd_color_fill_dma(point_t *p1, point_t *p2,
                                const uint16_t *color,
                                dma_complete_callback_t complete,
                                void *user_arg) {
  rect_t rect;
  uint32_t point;
  dma_result_t result;

  if (p1 == NULL || p2 == NULL || color == NULL ||
      p1->x > p2->x || p1->y > p2->y || p2->x >= lcddev.width ||
      p2->y >= lcddev.height)
    return DMA_ERROR_INVALID_PARAM;

  rect.x = p1->x;
  rect.y = p1->y;
  rect.width = p2->x - p1->x + 1;
  rect.height = p2->y - p1->y + 1;
  point = (uint32_t)rect.width * rect.height;

  lcd_set_window(&rect);
  lcd_write_ram_prepare();

  lcd_dma.complete_callback = complete;
  lcd_dma.error_callback = lcd_dma_error_callback;
  lcd_dma.user_arg = user_arg;
  result = dma_start(&lcd_dma, (uint32_t)color, (uint32_t)&LCD->ram,
                     (uint16_t)point);
  if (result != DMA_OK)
    lcd_dma.complete_callback = NULL;
  return result;
}
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
  uint16_t ex = rect->x + rect->width - 1;
  uint16_t ey = rect->y + rect->height - 1;

  // 设置窗口
  lcd_write_regno(lcddev.setxcmd);
  lcd_write_data(rect->x >> 8);
  lcd_write_regno(lcddev.setxcmd + 1);
  lcd_write_data(rect->x & 0xFF);
  lcd_write_regno(lcddev.setxcmd + 2);
  lcd_write_data(ex >> 8);
  lcd_write_regno(lcddev.setxcmd + 3);
  lcd_write_data(ex & 0xFF);

  lcd_write_regno(lcddev.setycmd);
  lcd_write_data(rect->y >> 8);
  lcd_write_regno(lcddev.setycmd + 1);
  lcd_write_data(rect->y & 0xFF);
  lcd_write_regno(lcddev.setycmd + 2);
  lcd_write_data(ey >> 8);
  lcd_write_regno(lcddev.setycmd + 3);
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
