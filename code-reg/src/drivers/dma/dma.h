#ifndef __DMA_H__
#define __DMA_H__

#include "type.h"

/**
 * @brief DMA 控制器编号
 */
typedef enum {
  DMA_CONTROLLER_1 = 1,
  DMA_CONTROLLER_2 = 2,
} dma_controller_t;

/**
 * @brief DMA 通道编号
 *
 * 通道编号使用芯片手册中的编号，从 1 开始。
 */
typedef enum {
  DMA_CHANNEL_1 = 1,
  DMA_CHANNEL_2,
  DMA_CHANNEL_3,
  DMA_CHANNEL_4,
  DMA_CHANNEL_5,
  DMA_CHANNEL_6,
  DMA_CHANNEL_7,
} dma_channel_id_t;

/**
 * @brief DMA 数据传输方向
 */
typedef enum {
  /** 内存到内存，例如内存缓冲区传输到 FSMC 映射地址 */
  DMA_DIR_MEMORY_TO_MEMORY,
  /** 内存到外设，例如内存发送到 SPI */
  DMA_DIR_MEMORY_TO_PERIPHERAL,
  /** 外设到内存，例如 ADC 采样到内存 */
  DMA_DIR_PERIPHERAL_TO_MEMORY,
} dma_direction_t;

/**
 * @brief DMA 单次传输的数据宽度
 */
typedef enum {
  DMA_WIDTH_8BIT,
  DMA_WIDTH_16BIT,
  DMA_WIDTH_32BIT,
} dma_width_t;

/**
 * @brief DMA 通道状态
 */
typedef enum {
  DMA_STATE_UNINITIALIZED,
  DMA_STATE_READY,
  DMA_STATE_BUSY,
  DMA_STATE_ERROR,
} dma_state_t;

/**
 * @brief DMA 操作结果
 */
typedef enum {
  DMA_OK,
  DMA_ERROR_INVALID_PARAM,
  DMA_ERROR_BUSY,
  DMA_ERROR_NOT_READY,
} dma_result_t;

/**
 * @brief DMA 传输完成回调
 *
 * 回调函数在 DMA 中断中执行，不应执行耗时操作。
 */
typedef void (*dma_complete_callback_t)(void *user_arg);

/**
 * @brief DMA 传输错误回调
 *
 * 回调函数在 DMA 传输错误中断中执行。
 */
typedef void (*dma_error_callback_t)(void *user_arg);

/**
 * @brief DMA 通道配置
 */
typedef struct {
  /** DMA 控制器编号 */
  dma_controller_t controller;
  /** DMA 通道编号 */
  dma_channel_id_t channel;
  /** 数据传输方向 */
  dma_direction_t direction;
  /** 源数据宽度 */
  dma_width_t source_width;
  /** 目标数据宽度 */
  dma_width_t destination_width;
  /** 源地址是否递增 */
  bool source_increment;
  /** 目标地址是否递增 */
  bool destination_increment;
  /** 是否使用循环传输 */
  bool circular;
  /** 通道优先级，使用 DMA_PL_* 配置 */
  uint32_t priority;
  /** 中断优先级 */
  ir_pri_t interrupt_priority;
} dma_config_t;

/**
 * @brief DMA 通道对象
 *
 * 一个对象对应一个硬件 DMA 通道，使用前必须保持对象一直有效。
 */
typedef struct {
  /** 通道配置 */
  dma_config_t config;
  /** 当前通道状态 */
  volatile dma_state_t state;
  /** 当前传输源地址 */
  uint32_t source;
  /** 当前传输目标地址 */
  uint32_t destination;
  /** 当前传输数量 */
  uint16_t count;
  /** 传输完成回调 */
  dma_complete_callback_t complete_callback;
  /** 传输错误回调 */
  dma_error_callback_t error_callback;
  /** 回调函数用户参数 */
  void *user_arg;
} dma_t;

/**
 * @brief 初始化 DMA 通道
 *
 * @param dma DMA 通道对象
 * @return DMA_OK 表示成功，否则返回错误原因
 */
dma_result_t dma_init(dma_t *dma);

/**
 * @brief 启动一次 DMA 传输
 *
 * @param dma 已初始化的 DMA 通道对象
 * @param source 源地址
 * @param destination 目标地址
 * @param count 传输数量，单位由数据宽度决定
 * @return DMA_OK 表示启动成功，否则返回错误原因
 */
dma_result_t dma_start(dma_t *dma, uint32_t source, uint32_t destination,
                        uint16_t count);

/**
 * @brief 停止 DMA 传输
 *
 * 只有 BUSY 状态的通道需要停止，停止后通道回到 READY 状态。
 *
 * @param dma DMA 通道对象
 * @return DMA_OK 表示成功，否则返回错误原因
 */
dma_result_t dma_stop(dma_t *dma);

/**
 * @brief 查询 DMA 通道当前状态
 *
 * @param dma DMA 通道对象
 * @return 当前 DMA 通道状态
 */
dma_state_t dma_get_state(const dma_t *dma);

#endif
