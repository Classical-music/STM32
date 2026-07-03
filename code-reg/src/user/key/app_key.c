#include "app_key.h"
#include "beep/task_beep.h"
#include "event.h"
#include "led/task_led.h"

static void event_handler_key_pressed(const event_t *evt);

void app_key_init(void) {
  key_init(); // 初始化 key
  ledInit();  // 初始化 ledTask
  beepInit(); // 初始化 beepTask

  // 注册 key 事件
  event_register(EVT_ID_KEY_PRESSED, event_handler_key_pressed);
}

static void event_handler_key_pressed(const event_t *evt) {
  key_id_t id = (key_id_t)evt->param;
  switch (id) {
  case KEY_UP:
    setLedMode(LED_MODE_BREATH);
    break;
  case KEY_DOWN:
    setLedMode(LED_MODE_FLOW);
    break;
  case KEY_LEFT:
    setLedMode(LED_MODE_NORMAL);
    playBeep(20);
    break;
  case KEY_RIGHT:
    setLedMode(LED_MODE_NORMAL);
    playBeep(40);
    break;

  default:
    break;
  }
}
