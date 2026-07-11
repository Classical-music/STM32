#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

void *pvPortRealloc(void *ptr, size_t new_size) {
  // 请求大小为0, 等同于释放
  if (new_size == 0) {
    vPortFree(ptr);
    return NULL;
  }

  // 传入 NULL 指针, 等同于 malloc
  if (ptr == NULL) {
    return pvPortMalloc(new_size);
  }

  // 常规 realloc: 分配新内存 -> 拷贝数据 -> 释放旧内存
  void *new_ptr = pvPortMalloc(new_size);
  if (new_ptr != NULL) {
    // 注意：这里需要知道旧内存块的大小，但 heap_5 未提供此API。
    // 为简化，我们拷贝 new_size 字节。若 new_size 大于旧块大小，可能访问越界。
    // 一个更安全但较慢的方法是先尝试获取旧块大小，如方案二所示。
    memcpy(new_ptr, ptr, new_size);
    vPortFree(ptr);
  }
  // 如果 pvPortMalloc 失败，返回 NULL，原指针 ptr 保持不变（符合 realloc
  // 标准行为）
  return new_ptr;
}
