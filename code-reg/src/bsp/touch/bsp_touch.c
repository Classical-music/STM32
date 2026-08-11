#include "bsp_touch.h"

void touch_init(void) {
  touch.init(); //
}

bool touch_is_pressed(void) {
  return touch.is_pressed(); //
}

void touch_get_xy(int32_t *x, int32_t *y) {
  touch.get_xy(x, y); //
}
