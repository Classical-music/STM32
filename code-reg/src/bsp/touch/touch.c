#include "touch.h"

void touch_init(void) {
  touch.init(); //
}

bool touch_read(uint16_t *x, uint16_t *y) {
  return touch.read(x, y); //
}
