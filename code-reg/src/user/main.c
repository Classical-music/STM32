#include "app/task_app.h"
#include "lcd/lcd.h"
#include "sram/bsp_sram.h"

// 内部SRAM 64KB, 预留外设栈、全局变量后, 分配56KB=0xE000
#define HEAP5_START ((uint8_t *)0x20002000) // 偏移8k
#define HEAP5_SIZE ((size_t)(0xC000))       // 48K

#define ESRAM_START ((uint8_t *)ESRAM_BASE)

const HeapRegion_t xHeapRegions[] = {
    {HEAP5_START, HEAP5_SIZE}, // 内部sram
    {ESRAM_START, ESRAM_SIZE}, // 外部sram
    {NULL, 0}                  //
};

int main(void) {
  vPortDefineHeapRegions(xHeapRegions);

  lcd_init(); // 初始化 LCD

  appTaskInit(); //
}
