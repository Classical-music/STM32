#ifndef __FreeRTOS_CONFIG_H__
#define __FreeRTOS_CONFIG_H__

#include "system_stm32f10x.h"
// 系统时钟
#define configCPU_CLOCK_HZ (72000000UL)
// 系统心跳中断频率 1ms
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

// 栈大小单位 word(4B)
#define configMINIMAL_STACK_SIZE ((unsigned short)256)

// heap_5 仅管理内部 SRAM 的 48KB 区域，实际起始地址在 main.c 中定义。
#define configTOTAL_HEAP_SIZE ((size_t)(0xC000))
#define configUSE_HEAP_5 1

// 任务优先级
#define configMAX_PRIORITIES (10)

// 中断优先级
#define configPRIO_BITS 4 // NVIC 中断优先级存储位数, STM32F1 全系4bit
// 最低中断优先级, 4bit最大值15
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
// 调用 freeRTOS-API的最大优先级, <=5 可调用, >5不可调用
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
// 内核使用的阈值
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                                   \
  configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

// 调度器基础行为
// 1-抢占式调度 0-协作式调度(任务必须主动调用vTaskDelay/阻塞, 才释放CPU)
#define configUSE_PREEMPTION 1
// 是否开启时间片轮转 1-开启 0-不开启
#define configUSE_TIME_SLICING 1
// 启用空闲任务钩子
#define configUSE_IDLE_HOOK 0 // 暂时关闭
// Tick 钩子, 一般关闭, 高精度计时再开
#define configUSE_TICK_HOOK 1
// 低功耗特殊HOOK, 普通F1不用
#define configUSE_PASSIVE_IDLE_HOOK 0
// 启用任务通知 (轻量信号量, 比二值信号量省内存)
#define configUSE_TASK_NOTIFICATIONS 1
// 每个任务通知数组长度, 1够用, 多组独立通知再加大
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1

// 栈/内存调试HOOK
// 0-关闭栈溢出检测 1-仅任务切换时校验尾部标记 2-任务入口+切换双重校验
// 调试阶段开2, 量产阶段关闭
// 开启后必须实现钩子: vApplicationStackOverflowHook
#define configCHECK_FOR_STACK_OVERFLOW 0
// 1-堆内存分配失败时进入钩子 vApplicationMallocFailedHook
#define configUSE_MALLOC_FAILED_HOOK 0

// 软件定时器配置
// 启用软定时器, 基于任务实现, 不占用硬件定时器
#define configUSE_TIMERS 1
// 定时器任务优先级, 高于空闲任务(0) 低于业务核心任务
#define configTIMER_TASK_PRIORITY (2)
// 定时器命令队列深度
#define configTIMER_QUEUE_LENGTH 10
// 定时器任务栈大小
#define configTIMER_TASK_STACK_DEPTH 256

// 信号量
#define configUSE_MUTEXES 1             // 互斥锁
#define configUSE_RECURSIZE_MUTEXES 1   // 递归互斥锁
#define configUSE_COUNTING_SEMAPHORES 1 // 计数信号量

// 队列注册表, 调试器查看队列名称, 不需要调试器时, 设置未0
#define configQUEUE_REGISTRY_SIZE 8
#define configUSE_QUEUE_SETS 0 // 队列组, 极少使用, 关闭

// 延时&阻塞
#define INCLUDE_vTaskDelay 1 // 启用 vTaskDelay
// 启用 vTaskDelayUntil 固定周期任务
#define configUSE_VTASKDELAY_UNTIL 1

#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_xTaskGetHandle 1 // 按任务名取句柄(memtest 时序扫描挂起 GUI 用)

// 内存分配&任务创建
#define configSUPPORT_DYNAMIC_ALLOCATION 1 // 启用堆内存
#define configSUPPORT_STATIC_ALLOCATION 0  // 不启用静态分配
// 向后兼容，统一类型名 QueueHandle_t
#define configENABLE_BACKWARD_COMPATIBILITY 1

// 高级功能
// 低功耗无嘀嗒休眠
#define configUSE_TICKLESS_IDLE 0

#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler
#define xPortSysTickHandler SysTick_Handler

// 自定义 realloc
extern void *pvPortRealloc(void *ptr, size_t new_size);

#endif
