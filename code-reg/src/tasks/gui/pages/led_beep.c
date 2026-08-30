#include "led_beep.h"
#include "../components/ui_button.h"
#include "beep/task_beep.h"

static void led_beep_btn_event_cb(lv_event_t *event) {
  (void)event;
  playBeep(20);
}

void view_led_beep_init(void *params) {
  lv_obj_t *button;

  (void)params;

  button = ui_button_create(lv_scr_act(), "LED / Beep");

  lv_obj_set_size(button, 120, 50);
  lv_obj_center(button);

  lv_obj_add_event_cb(button, led_beep_btn_event_cb, LV_EVENT_CLICKED, NULL);
}
