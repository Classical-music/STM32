#ifndef __FSMC_H__
#define __FSMC_H__

#include "type.h"

// 数据总线宽度
typedef enum {
  fsmc_mwid_8 = 0x00,
  fsmc_mwid_16 = 0x10,
} fsmc_mwid_t;

typedef struct {
  FSMC_Bank1_TypeDef *bank;
  FSMC_Bank1E_TypeDef *bankE;
  uint8_t neId;
  fsmc_mwid_t mwid;
} fsmc_ctx_t;

void fsmc_init(fsmc_ctx_t *ctx);

#define FSMC_INIT(ctx) fsmc_init(&ctx)

#endif
