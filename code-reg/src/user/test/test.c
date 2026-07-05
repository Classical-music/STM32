#include "test.h"

extern void eeprom_test(void);
extern void flash_test(void);
extern void lcd_test(void);

void test(void) {
  eeprom_test();
  flash_test();
  lcd_test();
}
