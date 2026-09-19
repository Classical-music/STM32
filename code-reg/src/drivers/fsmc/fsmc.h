#ifndef __FSMC_H__
#define __FSMC_H__

#include "type.h"

// 数据总线宽度
typedef enum {
  fsmc_mwid_8 = 0x00,
  fsmc_mwid_16 = 0x10,
} fsmc_mwid_t;

// NE3(外部 SRAM) 默认时序, 单位 HCLK(72MHz 下 1 HCLK ≈ 13.9ns)
// {0,1} 为 memtest scan 实测选型: 访问总周期 3 HCLK, 较原 {0,2} 提速约 10%,
// 数据校验连续通过. 注意数据窗口约 13.9ns, 对 15ns 级 SRAM 属零裕量运行.
#define FSMC_NE3_DEF_ADDSET 0u
#define FSMC_NE3_DEF_DATAST 1u

// for-lcd
void fsmc_ne4_init(void);

// for-sram
void fsmc_ne3_init(void);

// 运行时修改 NE3 读写时序(EXTMOD=0, 读写同时序), 用于 memtest 调参
void fsmc_ne3_set_timing(uint32_t addset, uint32_t datast);

#endif
