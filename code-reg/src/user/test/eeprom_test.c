#include "eeprom_test.h"
#include "eeprom/eeprom.h"

void eeprom_test(void) {
  uint8_t ch = eeprom_read_byte(0);
  printf("readByte[%x] = %x\n", 0, ch);
  eeprom_write_byte(0, ch + 1);
  ch = eeprom_read_byte(0);
  printf("readByte[%x] = %x\n", 0, ch);
}
