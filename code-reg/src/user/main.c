#include "app/task_app.h"
#include "board.h"
#include "sram/bsp_sram.h"
#include "test/test.h"

// 内部 SRAM 前 12KB 由链接器分配给全局变量、静态变量和启动栈。
// FreeRTOS heap 使用随后 48KB，末尾 4KB 作为安全余量。
#define HEAP5_START ((uint8_t *)0x20003000)
#define HEAP5_SIZE ((size_t)(0xC000))

// heap_5 仅管理内部 SRAM；LVGL 使用独立的外部 SRAM 内存池。
const HeapRegion_t xHeapRegions[] = {
    {HEAP5_START, HEAP5_SIZE},
    {NULL, 0}
};

int main(void) {
  // LVGL 内存池位于外部 SRAM，必须先初始化 FSMC，才能访问该区域。
  bsp_sram_init();

  // 在创建任何任务、队列等动态对象之前注册 heap_5 内存区域。
  vPortDefineHeapRegions(xHeapRegions);

  // 外部 SRAM 已初始化，board_init() 不再重复初始化 SRAM。
  board_init();
  test();

  appTaskInit();
}
