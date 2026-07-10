#include "sram/bsp_sram.h"
#include "test.h"

// 测试 sram 读写
void sram_test(void) {
  // 测试8bit读写
  for (uint8_t i = 0; i < 10; i++) {
    SRAM_WRITE8(i, i + '0');
  }
  for (uint8_t i = 0; i < 10; i++) {
    printf("sram8[%d] = %c\n", i, SRAM_READ8(i));
  }
  // 测试16bit读写
  for (uint8_t i = 0; i < 10; i += 2) {
    SRAM_WRITE16(i, (i + 1) << 8 | i);
  }
  for (uint8_t i = 0; i < 10; i += 1) {
    printf("sram16[%d] = %x\n", i, SRAM_READ16(i));
  }
}
