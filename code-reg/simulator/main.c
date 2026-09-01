#include <SDL.h>

#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl.h"
#include "views/views.h"

int main(void) {
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  views_init();

  while (!lv_port_indev_should_quit()) {
    lv_port_indev_process_events();
    lv_timer_handler();
    SDL_Delay(5);
  }

  lv_port_disp_deinit();
  return 0;
}
