#include "board.h"
#include "eeprom/eeprom.h"
#include "flash/flash.h"
#include "lcd/lcd.h"
#include "wwdg.h"

void board_init(void) {
  // wwdg_init(0x7F, 0x5F, WWDG_WDGTB_8); // 初始化看门狗
  lcd_init();    // 初始化 LCD
  flash_init();  // 初始化 Flash
  eeprom_init(); // 初始化 EEPROM
}
