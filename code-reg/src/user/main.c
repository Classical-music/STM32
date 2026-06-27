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
  uint8_t ch = eeprom_read_byte(0);
  eeprom_write_byte(0, ++ch);
  // eeprom_write_byte(1, 'e');
  // eeprom_write_byte(2, 'f');
  printf("eeprom read byte: %c\n", eeprom_read_byte(0)
         // , eeprom_read_byte(1), eeprom_read_byte(2)
  );

  while (1) {
    timer_tick();
    event_dispatch();
  }
}
