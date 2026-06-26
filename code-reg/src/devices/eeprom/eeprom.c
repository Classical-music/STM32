#include "eeprom.h"

extern eeprom_t eeprom_24c02;

static eeprom_t *eeprom_ptr = &eeprom_24c02;

void eeprom_init(void) {
  eeprom_ptr->init(); //
}
uint8_t eeprom_read_byte(uint8_t addr) {
  return eeprom_ptr->read_byte(addr); //
}

void eeprom_write_byte(uint8_t addr, uint8_t data) {
  eeprom_ptr->write_byte(addr, data); //
}
void eeprom_read(uint8_t addr, uint8_t *buff, uint8_t size) {
  eeprom_ptr->read(addr, buff, size); //
}
void eeprom_write(uint8_t addr, uint8_t *buff, uint8_t size) {
  eeprom_ptr->write(addr, buff, size);
}
