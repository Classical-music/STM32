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

void touch_set_irq_callback(touch_irq_callback_t callback) {
  touch.set_irq_callback(callback);
}

void touch_process(void) { touch.process(); }
