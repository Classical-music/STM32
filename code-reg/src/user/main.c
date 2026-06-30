#include "board.h"
#include "cmd/cmd.h"
#include "eeprom/eeprom.h"
#include "event.h"
#include "flash/flash.h"
#include "key/app_key.h"
#include "lcd/lcd.h"
#include "test/test.h"
#include "timer.h"
#include "tpad.h"
#include "usart.h"
#include <stdio.h>

int main() {
  board_init();

  event_init();
  timer_init();
  app_key_init();
  cmd_init();
  tpad_init();
  lcd_init();
  EEPROM_INIT();
  flash_init();

  test();

  while (1) {
    timer_tick();
    event_dispatch();
  }
}
