#include "board.h"
#include "cmd/cmd.h"
#include "eeprom/eeprom.h"
#include "event.h"
#include "flash/flash.h"
#include "key/app_key.h"
#include "lcd.h"
#include "timer.h"
#include "tpad.h"
#include "usart.h"
#include <stdio.h>

void flash_erase_sector(uint32_t saddr);
extern void flash_write_page(uint32_t addr, uint8_t *buff, uint16_t size);
void read_w25q128(uint32_t addr, uint8_t *buff, uint16_t size);

int main() {
  board_init();

  event_init();
  timer_init();
  app_key_init();
  cmd_init();
  tpad_init();
  // lcd_init();
  // EEPROM_INIT();
  flash_init();

  // 擦除
  // char str[] = "hello, world";
  // FLASH_WRITE(0, str);
  // flash_write_page(0, "h", 1);
  char buff[16] = {0};
  read_w25q128(0, buff, 1);
  flash_erase_sector(0);
  read_w25q128(0, buff, 1);
  // FLASH_READ(0, buff);
  // printf("read flash: %s\n", buff);

  while (1) {
    timer_tick();
    event_dispatch();
  }
}
