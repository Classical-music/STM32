#include "dma.h"

static const dma_t *g_dma_ctxs[2][7];

// 初始化一个dma通道
bool dam_init(dma_t *dma) {
  // RCC
  if (dma->dmaId == 1)
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  else if (dma->dmaId == 2)
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
  else
    return false;

  // 通道初始化
  DMA_Channel_TypeDef *cha = dma->cha;

  // 通道控制寄存器
  cha->CCR = 0; // 清空配置

  // 储存器->存储器模式
  if (dma->mem2mem)
    cha->CCR |= DMA_CCR1_MEM2MEM;

  cha->CCR |= dma->pl; // 通道优先级

  // 数据宽度
  cha->CCR |= dma->msize | dma->psize;

  // 增量模式
  if (dma->minc) {
    cha->CCR |= DMA_CCR1_MINC;
  }
  if (dma->pinc) {
    cha->CCR |= DMA_CCR1_PINC;
  }

  if (dma->cir) {
    cha->CCR |= DMA_CCR1_CIRC; // 开启循环模式
  }

  if (dma->toP) {
    // 数据传输方向
    cha->CCR |= DMA_CCR1_DIR; // -> 外设
  }

  // 中断
  if (dma->ht_callback) {
    cha->CCR |= DMA_CCR1_HTIE;
  }
  if (dma->tc_callback) {
    cha->CCR |= DMA_CCR1_TCIE;
  }

  // NVIC
  NVIC_EnableIRQ(dma->irq);
  NVIC_SetPriority(dma->irq, dma->pri);

  // 将DMA&通道注册, 用于中断回调
  g_dma_ctxs[dma->dmaId][dma->chaId] = dma;

  return true;
}
// 启动一次dma
bool dma_start(dma_t *dma, uint32_t src, uint32_t dst, uint16_t length) {
  DMA_Channel_TypeDef *cha = dma->cha;
  if (dma->toP) {
    cha->CMAR = src; // 存储器 -> 外设
    cha->CPAR = dst;
  } else {
    cha->CMAR = dst; // 外设 -> 存储器
    cha->CPAR = src;
  }
  cha->CNDTR = length;     // 发送长度
  cha->CCR |= DMA_CCR1_EN; // 启动通道

  return true;
}
// 停止dma传输
void dma_stop(dma_t *dma) {
  dma->cha->CCR &= ~DMA_CCR1_EN; // 停止DMA通道
}

#define DMA_IRQHandler(dmaId, chaId)                                           \
  do {                                                                         \
    DMA_TypeDef *dma = DMA##dmaId;                                             \
    uint32_t isr = dma->ISR;                                                   \
    if (isr & DMA_ISR_TCIF##chaId) {                                           \
      dma->IFCR |= DMA_IFCR_CTCIF##chaId;                                      \
      g_dma_ctxs[dmaId - 1][chaId - 1]->tc_callback();                         \
    }                                                                          \
    if (isr & DMA_ISR_HTIF##chaId) {                                           \
      dma->IFCR |= DMA_IFCR_CHTIF##chaId;                                      \
      g_dma_ctxs[dmaId - 1][chaId - 1]->ht_callback();                         \
    }                                                                          \
  } while (0)

void DMA1_Channel1_IRQHandler(void) {
  DMA_IRQHandler(1, 1); //
}
void DMA1_Channel2_IRQHandler(void) {
  DMA_IRQHandler(1, 2); //
}
void DMA1_Channel3_IRQHandler(void) {
  DMA_IRQHandler(1, 3); //
}
void DMA1_Channel4_IRQHandler(void) {
  DMA_IRQHandler(1, 4); //
}
void DMA1_Channel5_IRQHandler(void) {
  DMA_IRQHandler(1, 5); //
}
void DMA1_Channel6_IRQHandler(void) {
  DMA_IRQHandler(1, 6); //
}
void DMA1_Channel7_IRQHandler(void) {
  DMA_IRQHandler(1, 7); //
}
void DMA2_Channel1_IRQHandler(void) {
  DMA_IRQHandler(2, 1); //
}
void DMA2_Channel2_IRQHandler(void) {
  DMA_IRQHandler(2, 2); //
}
void DMA2_Channel3_IRQHandler(void) {
  DMA_IRQHandler(2, 3); //
}
void DMA2_Channel4_5_IRQHandler(void) {
  DMA_IRQHandler(2, 4); //
  DMA_IRQHandler(2, 5); //
}
