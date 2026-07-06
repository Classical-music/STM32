#ifndef __DMA_H__
#define __DMA_H__

#include "type.h"

// dma外设

// 通道优先级
typedef enum {
  DMA_PL_LOW = 0x0000,     // 低
  DMA_PL_MID = 0x1000,     // 中
  DMA_PL_HIGH = 0x2000,    // 高
  DMA_PL_HIGHest = 0x3000, // 最高
} dma_pl_t;

// 存储器/外设数据宽度
typedef enum {
  DMA_PSIZE_8 = 0x000,  // 字节
  DMA_PSIZE_16 = 0x100, // 2字节
  DMA_PSIZE_32 = 0x200, // 4字节
} dma_psize_t;
typedef enum {
  DMA_MSIZE_8 = 0x000,  // 字节
  DMA_MSIZE_16 = 0x400, // 2字节
  DMA_MSIZE_32 = 0x800, // 4字节
} dma_msize_t;

typedef struct {
  uint8_t dmaId;            // 0~1
  uint8_t chaId;            // 0~6
  DMA_Channel_TypeDef *cha; // dma 通道

  IRQn_Type irq; // NIIC 优先级参数
  ir_pri_t pri;

  // CCR
  bool mem2mem;      // 存储器到存储器模式
  dma_pl_t pl;       // 通道优先级
  dma_msize_t msize; // 存储器数据宽度
  dma_psize_t psize; // 外设数据宽度
  bool minc;         // 存储器增量模式
  bool pinc;         // 外设增量模式
  bool cir;          // 循环模式
  bool toP;          // 传输方向 true-to外设 false-to存储器

  void (*ht_callback)(void); // 半传输中断回调
  void (*tc_callback)(void); // 传输中断回调
} dma_t;

// 初始化一个dma通道
bool dam_init(dma_t *dma);
// 启动一次dma
bool dma_start(dma_t *dma, uint32_t src, uint32_t dst, uint16_t length);
// 停止dma传输
void dma_stop(dma_t *dma);

#endif
