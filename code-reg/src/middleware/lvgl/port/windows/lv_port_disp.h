#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define LV_PORT_DISP_HOR_RES 480
#define LV_PORT_DISP_VER_RES 800

void lv_port_disp_init(void);
void lv_port_disp_deinit(void);
void disp_enable_update(void);
void disp_disable_update(void);

#ifdef __cplusplus
}
#endif

#endif
