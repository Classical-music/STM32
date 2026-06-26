#include "board.h"
#include "cmd/cmd.h"
#include "eeprom/eeprom.h"
#include "event.h"
#include "key/app_key.h"
#include "lcd.h"
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
  eeprom_init();
  eeprom_write_byte(0, 'a');
  eeprom_write_byte(1, 'b');
  eeprom_write_byte(2, 'c');
  printf("eeprom read byte: %c %c %c\n", eeprom_read_byte(0),
         eeprom_read_byte(1), eeprom_read_byte(2));

  while (1) {
    timer_tick();
    event_dispatch();
  }
}
