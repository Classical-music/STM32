#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "type.h"

typedef struct {
  void (*init)(void);
  uint8_t (*read_byte)(uint8_t addr);
  void (*write_byte)(uint8_t addr, uint8_t data);
  void (*read)(uint8_t addr, uint8_t *buff, uint16_t size);
  void (*write)(uint8_t addr, uint8_t *buff, uint16_t size);
} eeprom_t;

void eeprom_init(void);
uint8_t eeprom_read_byte(uint8_t addr);
void eeprom_write_byte(uint8_t addr, uint8_t data);
void eeprom_read(uint8_t addr, uint8_t *buff, uint16_t size);
void eeprom_write(uint8_t addr, uint8_t *buff, uint16_t size);

#define EEPROM_INIT eeprom_init

#define EEPROM_RB eeprom_read_byte
#define EEPROM_WB eeprom_write_byte

#define EEPROM_READ(addr, buff)                                                \
  eeprom_read((uint8_t)addr, (uint8_t *)buff, sizeof(buff))
#define EEPROM_WRITE(addr, buff)                                               \
  eeprom_write((uint8_t)addr, (uint8_t *)buff, sizeof(buff))

#endif
