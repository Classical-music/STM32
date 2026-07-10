#include "app/task_app.h"
#include "sram/bsp_sram.h"

// 内部SRAM 64KB, 预留外设栈、全局变量后, 分配56KB=0xE000
#define HEAP5_START ((uint8_t *)0x20001000) // 偏移4k
#define HEAP5_SIZE ((size_t)(0xE000))       // 56K

#define ESRAM_START ((uint8_t *)ESRAM_BASE)

const HeapRegion_t xHeapRegions[] = {
    {HEAP5_START, HEAP5_SIZE}, // 内部sram
    {ESRAM_START, ESRAM_SIZE}, // 外部sram
    {NULL, 0}                  //
};

int main(void) {
  vPortDefineHeapRegions(xHeapRegions);

  appTaskInit(); //
}
