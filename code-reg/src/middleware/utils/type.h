#ifndef __TYPE_H__
#define __TYPE_H__

#include "FreeRTOS.h"
#include "stm32f10x.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])

#define __CONCAT(x, y) x##y
#define CONCAT(x, y) __CONCAT(x, y)

#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - offsetof(type, member)))

#define SWAP(a, b)                                                             \
  do {                                                                         \
    __typeof__(a) __temp = (a);                                                \
    (a) = (b);                                                                 \
    (b) = __temp;                                                              \
  } while (0)

// 中断优先级
typedef enum {
  IR_PRI_0 = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
  IR_PRI_WWDG,
  IR_PRI_USART,
  IR_PRI_TIM_CAP,
  IR_PRI_TOUCH, // 触摸屏中断
} ir_pri_t;

#endif
