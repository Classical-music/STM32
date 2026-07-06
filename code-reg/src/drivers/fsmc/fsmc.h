#ifndef __FSMC_H__
#define __FSMC_H__

#include "type.h"

// 数据总线宽度
typedef enum {
  fsmc_mwid_8 = 0x00,
  fsmc_mwid_16 = 0x10,
} fsmc_mwid_t;

void fsmc_ne4_init(void);

#endif
