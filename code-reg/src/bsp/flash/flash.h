#ifndef __W25Q128_H__
#define __W25Q128_H__

#include "type.h"

typedef struct {
  void (*init)(void);
  void (*write)(uint32_t addr, uint8_t *buff, uint16_t size);
  void (*read)(uint32_t addr, uint8_t *buff, uint16_t size);
} flash_t;

extern flash_t flash_m25q128;

void flash_init(void);

void flash_write(uint32_t addr, uint8_t *buff, uint16_t size);

void flash_read(uint32_t addr, uint8_t *buff, uint16_t size);

#define FLASH_WRITE(addr, buff) flash_write(addr, (uint8_t *)buff, sizeof(buff))

#define FLASH_READ(addr, buff) flash_read(addr, (uint8_t *)buff, sizeof(buff))

#endif
