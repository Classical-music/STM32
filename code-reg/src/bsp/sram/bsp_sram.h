#ifndef __BSP_SRAM_H__
#define __BSP_SRAM_H__

#include "type.h"

typedef struct {
  void (*init)(void);
} bsp_sram_t;

#endif
