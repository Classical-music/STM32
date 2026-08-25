#include "dma.h"

/* DMA 控制器和通道的最大数量。DMA2 实际只使用前 5 个通道。 */
#define DMA_CONTROLLER_COUNT 2
#define DMA_CHANNEL_COUNT 7
#define DMA_PRIORITY_MAX 3

/* 保存每个硬件通道对应的 DMA 对象，供中断处理函数查找。 */
// 通道DMA通道对象默认初始化为NULL, 仅当DMA通道初始化时才赋值
static dma_t *dma_channels[DMA_CONTROLLER_COUNT][DMA_CHANNEL_COUNT];

/* 根据控制器编号获取 DMA 控制器寄存器基地址。 */
static DMA_TypeDef *dma_get_controller(dma_controller_t controller) {
  if (controller == DMA_CONTROLLER_1) {
    return DMA1;
  }
  if (controller == DMA_CONTROLLER_2) {
    return DMA2;
  }
  return NULL;
}

/* 根据控制器和通道编号获取 DMA 通道寄存器基地址。 */
static DMA_Channel_TypeDef *dma_get_channel(dma_controller_t controller,
                                            dma_channel_id_t channel) {
  if (controller == DMA_CONTROLLER_1) {
    switch (channel) {
    case DMA_CHANNEL_1:
      return DMA1_Channel1;
    case DMA_CHANNEL_2:
      return DMA1_Channel2;
    case DMA_CHANNEL_3:
      return DMA1_Channel3;
    case DMA_CHANNEL_4:
      return DMA1_Channel4;
    case DMA_CHANNEL_5:
      return DMA1_Channel5;
    case DMA_CHANNEL_6:
      return DMA1_Channel6;
    case DMA_CHANNEL_7:
      return DMA1_Channel7;
    default:
      return NULL;
    }
  }

  if (controller == DMA_CONTROLLER_2) {
    switch (channel) {
    case DMA_CHANNEL_1:
      return DMA2_Channel1;
    case DMA_CHANNEL_2:
      return DMA2_Channel2;
    case DMA_CHANNEL_3:
      return DMA2_Channel3;
    case DMA_CHANNEL_4:
      return DMA2_Channel4;
    case DMA_CHANNEL_5:
      return DMA2_Channel5;
    default:
      return NULL;
    }
  }

  return NULL;
}

/* 根据通道编号和标志偏移计算 DMA 状态标志位。 */
static uint32_t dma_get_flag(dma_channel_id_t channel, uint32_t offset) {
  return (uint32_t)1 << (offset + ((channel - 1) * 4));
}

/* 清除指定 DMA 通道的全局、完成、半传输和错误标志。 */
static void dma_clear_flags(dma_t *dma) {
  uint32_t flags = dma_get_flag(dma->config.channel, 0) |
                   dma_get_flag(dma->config.channel, 1) |
                   dma_get_flag(dma->config.channel, 2) |
                   dma_get_flag(dma->config.channel, 3);
  dma_get_controller(dma->config.controller)->IFCR = flags;
}

/* 禁止 DMA 通道，修改通道配置前必须先执行此操作。 */
static void dma_disable(dma_t *dma) {
  dma_get_channel(dma->config.controller, dma->config.channel)->CCR &=
      (uint16_t)~DMA_CCR1_EN;
}

/* 检查 DMA 对象、控制器、通道和基本配置是否合法。 */
static bool dma_is_valid(const dma_t *dma) {
  if (dma == NULL) {
    return false;
  }
  if (dma_get_controller(dma->config.controller) == NULL ||
      dma_get_channel(dma->config.controller, dma->config.channel) == NULL) {
    return false;
  }
  if (dma->config.source_width != dma->config.destination_width ||
      dma->config.priority > DMA_PRIORITY_MAX) {
    return false;
  }
  return true;
}

/* 处理单个 DMA 通道的传输错误和传输完成事件。 */
static void dma_handle_irq(dma_t *dma) {
  if (dma == NULL) {
    return;
  }

  DMA_TypeDef *controller = dma_get_controller(dma->config.controller);
  uint32_t error_flag = dma_get_flag(dma->config.channel, 3);
  uint32_t half_flag = dma_get_flag(dma->config.channel, 2);
  uint32_t complete_flag = dma_get_flag(dma->config.channel, 1);
  uint32_t status = controller->ISR;

  // 处理传输错误
  // 清除错误标志, 禁用通道, 更新状态为 DMA_STATE_ERROR
  // 调用错误回调函数
  if ((status & error_flag) != 0) {
    dma_clear_flags(dma);
    dma_disable(dma);
    dma->state = DMA_STATE_ERROR;
    if (dma->error_callback != NULL) {
      dma->error_callback(dma->user_arg);
    }
    return;
  }

  // 处理传输完成
  // 清除完成标志, 如果不是循环模式, 禁用通道, 更新状态为 DMA_STATE_READY
  // 调用完成回调函数
  if ((status & complete_flag) != 0) {
    dma_clear_flags(dma);
    if (!dma->config.circular) {
      dma_disable(dma);
    }
    dma->state = DMA_STATE_READY;
    if (dma->complete_callback != NULL) {
      dma->complete_callback(dma->user_arg);
    }
    return;
  }

  // 处理半传输
  // 清除半传输标志
  if ((status & half_flag) != 0) {
    dma_clear_flags(dma);
  }
}

/*
 * 根据 DMA 控制器和通道编号获取对应的 IRQ 编号。
 * DMA2 的通道4和通道5共用同一个 IRQ。
 */
static inline IRQn_Type dma_get_irq(dma_controller_t controller,
                                    dma_channel_id_t channel) {
  if (controller == DMA_CONTROLLER_1) {
    return (IRQn_Type)(DMA1_Channel1_IRQn + channel - 1);
  }

  if (channel <= DMA_CHANNEL_3) {
    return (IRQn_Type)(DMA2_Channel1_IRQn + channel - 1);
  }

  return DMA2_Channel4_5_IRQn;
}

/* 根据 DMA 控制器和通道配置对应的 NVIC 中断。 */
static void dma_init_irq(dma_t *dma) {
  IRQn_Type irq = dma_get_irq(dma->config.controller, dma->config.channel);

  /* 当前工程使用 CMSIS 的 NVIC 接口，不依赖旧版标准外设库的
   * NVIC_InitTypeDef 和 NVIC_Init。 */
  NVIC_SetPriority(irq, dma->config.interrupt_priority);
  NVIC_EnableIRQ(irq);
}

/*
 * 初始化 DMA 通道。
 *
 * 初始化过程中先关闭通道并清除历史标志，再写入固定配置。
 * 通道对象注册到全局表后，中断入口才能找到对应的对象。
 */
dma_result_t dma_init(dma_t *dma) {
  DMA_Channel_TypeDef *channel;
  uint32_t controller_index;
  uint32_t channel_index;

  if (!dma_is_valid(dma)) {
    return DMA_ERROR_INVALID_PARAM;
  }

  controller_index = dma->config.controller - 1;
  channel_index = dma->config.channel - 1;
  if (dma_channels[controller_index][channel_index] != NULL &&
      dma_channels[controller_index][channel_index] != dma) {
    return DMA_ERROR_BUSY;
  }

  // 启动 RCC
  if (dma->config.controller == DMA_CONTROLLER_1) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  } else {
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
  }

  // 获取通道对象
  channel = dma_get_channel(dma->config.controller, dma->config.channel);
  // 清空 DMA 标志
  channel->CCR &= (uint16_t)~DMA_CCR1_EN;
  dma_clear_flags(dma);

  // 设置数据宽度和通道优先级。
  // PSIZE 对应 CPAR，MSIZE 对应 CMAR。
  // 内存到外设时，源在 CMAR、目标在 CPAR，因此宽度需要交换。
  if (dma->config.direction == DMA_DIR_MEMORY_TO_PERIPHERAL) {
    channel->CCR = (uint16_t)((dma->config.destination_width << 10) |
                              (dma->config.source_width << 8) |
                              (dma->config.priority << 12));
  } else {
    // 外设到内存、内存到内存：源在 CPAR、目标在 CMAR。
    channel->CCR = (uint16_t)((dma->config.source_width << 10) |
                              (dma->config.destination_width << 8) |
                              (dma->config.priority << 12));
  }

  // 配置源地址和目标地址的递增模式。
  // 内存到外设时，源在 CMAR、目标在 CPAR。
  // 外设到内存和内存到内存时，源在 CPAR、目标在 CMAR。
  if (dma->config.direction == DMA_DIR_MEMORY_TO_PERIPHERAL) {
    if (dma->config.source_increment) {
      channel->CCR |= DMA_CCR1_MINC;
    }
    if (dma->config.destination_increment) {
      channel->CCR |= DMA_CCR1_PINC;
    }
  } else {
    if (dma->config.source_increment) {
      channel->CCR |= DMA_CCR1_PINC;
    }
    if (dma->config.destination_increment) {
      channel->CCR |= DMA_CCR1_MINC;
    }
  }
  // 设置循环模式
  if (dma->config.circular) {
    channel->CCR |= DMA_CCR1_CIRC;
  }
  /**
   * 设置传输方向
   * 内存到外设时, 源在 CMAR、目标在 CPAR
   * 外设到内存和内存到内存时, 源在 CPAR、目标在 CMAR
   */
  if (dma->config.direction == DMA_DIR_MEMORY_TO_PERIPHERAL) {
    channel->CCR |= DMA_CCR1_DIR;
  }
  // 设置内存到内存传输模式
  if (dma->config.direction == DMA_DIR_MEMORY_TO_MEMORY) {
    channel->CCR |= DMA_CCR1_MEM2MEM;
  }

  // 开启传输完成中断和传输错误中断
  channel->CCR |= DMA_CCR1_TCIE | DMA_CCR1_TEIE;
  dma_channels[controller_index][channel_index] = dma;
  dma->state = DMA_STATE_READY;
  dma_init_irq(dma);
  return DMA_OK;
}

/*
 * 启动一次 DMA 传输。
 *
 * 只有 READY 或 ERROR 状态允许重新装载地址和传输数量。
 * 通道处于 BUSY 状态时直接返回，避免覆盖当前传输。
 */
dma_result_t dma_start(dma_t *dma, uint32_t source, uint32_t destination,
                       uint16_t count) {
  DMA_Channel_TypeDef *channel;

  if (!dma_is_valid(dma) || source == 0 || destination == 0 || count == 0) {
    return DMA_ERROR_INVALID_PARAM;
  }
  if (dma->state == DMA_STATE_UNINITIALIZED) {
    return DMA_ERROR_NOT_READY;
  }
  if (dma->state == DMA_STATE_BUSY) {
    return DMA_ERROR_BUSY;
  }

  channel = dma_get_channel(dma->config.controller, dma->config.channel);
  // 每次启动 DMA 传输时, 先关闭通道, 再清除历史标志
  channel->CCR &= (uint16_t)~DMA_CCR1_EN;
  dma_clear_flags(dma);

  // 设置源地址和目标地址。
  // 内存到外设：CMAR 为源地址，CPAR 为目标地址。
  // 外设到内存：CPAR 为源地址，CMAR 为目标地址。
  // 内存到内存：在当前 DIR=0 配置下，CPAR 为源地址，CMAR 为目标地址。
  if (dma->config.direction == DMA_DIR_MEMORY_TO_PERIPHERAL) {
    channel->CPAR = destination;
    channel->CMAR = source;
  } else {
    channel->CPAR = source;
    channel->CMAR = destination;
  }
  // 设置传输数量
  channel->CNDTR = count;
  // 更新DMA对象的配置信息
  dma->source = source;
  dma->destination = destination;
  dma->count = count;
  dma->state = DMA_STATE_BUSY;
  // 启动 DMA 传输
  channel->CCR |= DMA_CCR1_EN;
  return DMA_OK;
}

/* 停止当前 DMA 传输，并恢复到 READY 状态。 */
dma_result_t dma_stop(dma_t *dma) {
  if (!dma_is_valid(dma)) {
    return DMA_ERROR_INVALID_PARAM;
  }
  if (dma->state == DMA_STATE_UNINITIALIZED) {
    return DMA_ERROR_NOT_READY;
  }

  dma_disable(dma);
  dma_clear_flags(dma);
  dma->state = DMA_STATE_READY;
  return DMA_OK;
}

/* 查询 DMA 通道当前的软件状态。 */
dma_state_t dma_get_state(const dma_t *dma) {
  if (dma == NULL) {
    return DMA_STATE_UNINITIALIZED;
  }
  return dma->state;
}

/* 为每个独立 DMA 通道生成对应的中断入口。 */
#define DMA_IRQ_HANDLER(controller, channel)                                   \
  void DMA##controller##_Channel##channel##_IRQHandler(void) {                 \
    dma_handle_irq(dma_channels[controller - 1][channel - 1]);                 \
  }

DMA_IRQ_HANDLER(1, 1)
DMA_IRQ_HANDLER(1, 2)
DMA_IRQ_HANDLER(1, 3)
DMA_IRQ_HANDLER(1, 4)
DMA_IRQ_HANDLER(1, 5)
DMA_IRQ_HANDLER(1, 6)
DMA_IRQ_HANDLER(1, 7)
DMA_IRQ_HANDLER(2, 1)
DMA_IRQ_HANDLER(2, 2)
DMA_IRQ_HANDLER(2, 3)

/* DMA2 通道4和通道5共用一个中断入口，需要分别处理。 */
void DMA2_Channel4_5_IRQHandler(void) {
  dma_handle_irq(dma_channels[1][3]);
  dma_handle_irq(dma_channels[1][4]);
}
