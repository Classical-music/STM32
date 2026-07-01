#include "lcd/lcd.h"
#include "test.h"

#define DrawHLine(x, y, len, color)                                            \
  do {                                                                         \
    point_t p = {x, y};                                                        \
    LCD_DRAW_HLINE(p, len, color);                                             \
  } while (0)

#define DrawLine(x1, y1, x2, y2, color)                                        \
  do {                                                                         \
    point_t p1 = {x1, y1}, p2 = {x2, y2};                                      \
    LCD_DRAW_LINE(p1, p2, color);                                              \
  } while (0)

#define DrawRect(x, y, width, height, color)                                   \
  do {                                                                         \
    rect_t rect = {x, y, width, height};                                       \
    LCD_DRAW_RECT(rect, color);                                                \
  } while (0)

#define DrawCircle(x, y, r, color)                                             \
  do {                                                                         \
    point_t p = {x, y};                                                        \
    LCD_DRAW_CIRCLE(p, r, color);                                              \
  } while (0)

void lcd_test(void) {
  DrawHLine(100, 100, 100, 0xFF0000);
  DrawLine(150, 150, 300, 250, 0x00FF00);
  DrawRect(0, 0, 150, 200, 0x0000FF);
  DrawCircle(0, 0, 100, 0xFFFF00);
}
