#include "app_key.h"
#include "app_beep.h"
#include "app_led.h"
#include "event.h"

static void event_handler_key_pressed(const event_t *evt);

void app_key_init(void) {
  key_init();     // 初始化 key
  app_led_init(); // 初始化 led
  beep_init();    // 初始化 Beep

  // 注册 key 事件
  event_register(EVT_ID_KEY_PRESSED, event_handler_key_pressed);
}

static void event_handler_key_pressed(const event_t *evt) {
  key_id_t id = (key_id_t)evt->param;
  switch (id) {
  case KEY_UP:
    // water_light_start();
    breathe_light_start();
    break;
  case KEY_DOWN:
    // water_light_stop();
    breathe_light_stop();
    break;
  case KEY_LEFT:
    app_beep_play();
    break;
  case KEY_RIGHT:
    app_beep_del(50);
    break;

  default:
    break;
  }
}
