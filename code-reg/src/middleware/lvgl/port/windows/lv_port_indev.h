#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "lvgl.h"

void lv_port_indev_init(void);
void lv_port_indev_process_events(void);
bool lv_port_indev_should_quit(void);

#ifdef __cplusplus
}
#endif

#endif
