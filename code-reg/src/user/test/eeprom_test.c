#include "eeprom_test.h"
#include "eeprom/eeprom.h"

// 测试字节读写
static void test_rw_byte(void) {
  uint8_t ch = eeprom_read_byte(0);
  printf("readByte[%x] = %x\n", 0, ch);
  ch = eeprom_read_byte(0);
  eeprom_write_byte(0, ch + 1);
  ch = eeprom_read_byte(0);
  printf("readByte[%x] = %x\n", 0, ch);
}

// 测试连续读
static void test_read(void) {
  eeprom_write_byte(0, 'a');
  eeprom_write_byte(1, '\0');
  uint8_t buff[256] = {0};
  EEPROM_READ(0, buff);
  printf("read %s\n", buff);
}

// 测试连续读写
static void test_rw(void) {
  uint8_t str[] = "hello world";
  EEPROM_WRITE(0, str);
  uint8_t buff[256] = {0};
  EEPROM_READ(0, buff);
  printf("rw:{%s}\n", buff);
}

// 测试超出写入范围
static void test_write_tail(void) {
  eeprom_write(0xFF, "a", 1);
  printf("read[%d] = %c\n", 255, eeprom_read_byte(255));
  eeprom_write(0xFF, "bc", 2);
  printf("read[%d] = %c\n", 255, eeprom_read_byte(255));
  eeprom_write(0xF0, "defghijklmn", 10);
  printf("read[%d] = %c\n", 255, eeprom_read_byte(255));
  uint8_t buff[256] = {0};
  EEPROM_READ(0, buff);
  printf("rw:{%s}\n", buff);
}

void eeprom_test(void) {
  test_rw_byte();
  test_read();
  test_rw();
  test_write_tail();
}
