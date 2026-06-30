#include "flash_test.h"
#include "flash/flash.h"

// 测试起始地址的读写
#define test_rw(addr, str)                                                     \
  do {                                                                         \
    uint8_t str2[] = str;                                                      \
    FLASH_WRITE(addr, str2);                                                   \
    uint8_t buff[256] = {0};                                                   \
    FLASH_READ(addr, buff);                                                    \
    printf("flash read: [%s]\n", buff);                                        \
  } while (0)

void flash_test(void) {
  // 普通地址读写
  test_rw(0x00, "hello, world");
  // 跨页地址读写
  test_rw(0xFE, "abcdefghijklmnopqrstuvwxyz");
  // 跨扇区地址读写
  test_rw(0xFFE, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}
