#ifndef __BEEP_H__
#define __BEEP_H__

#include "gpio.h"

void beep_init(void);

void beep_on(void);
void beep_off(void);

void beep_toggle(void);

#endif
