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
  EEPROM_INIT();
  EEPROM_WB(0, 'a');
  EEPROM_WB(1, 'b');
  EEPROM_WB(2, 'c');
  printf("eeprom read byte: %c %c %c\n", EEPROM_RB(0), EEPROM_RB(1),
         EEPROM_RB(2));
  char str[] = "hello, world!";
  EEPROM_WRITE(0, str);
  char buf[16] = {0};
  EEPROM_READ(0, buf);
  printf("eeprom read: %s\n", buf);

  while (1) {
    timer_tick();
    event_dispatch();
  }
}
