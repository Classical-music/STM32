#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "type.h"

#if USE_FREERTOS == 0

void systick_init(uint16_t freq);

#endif

#endif
