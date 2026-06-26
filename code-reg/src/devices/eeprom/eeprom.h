#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "type.h"

typedef struct {
  void (*init)(void);
  uint8_t (*read_byte)(uint8_t addr);
  void (*write_byte)(uint8_t addr, uint8_t data);
  void (*read)(uint8_t addr, uint8_t *buff, uint8_t size);
  void (*write)(uint8_t addr, uint8_t *buff, uint8_t size);
} eeprom_t;

void eeprom_init(void);
uint8_t eeprom_read_byte(uint8_t addr);
void eeprom_write_byte(uint8_t addr, uint8_t data);
void eeprom_read(uint8_t addr, uint8_t *buff, uint8_t size);
void eeprom_write(uint8_t addr, uint8_t *buff, uint8_t size);

#endif
