#include "board.h"
#include "eeprom/eeprom.h"
#include "flash/flash.h"
#include "lcd/lcd.h"
#include "sram/bsp_sram.h"
#include "wwdg/wwdg.h"

void board_init(void) {
  // wwdg_init(0x7F, 0x5F, WWDG_WDGTB_8); // 初始化看门狗
  flash_init();    // 初始化 Flash
  eeprom_init();   // 初始化 EEPROM
  bsp_sram_init(); // 初始化 sram
}
