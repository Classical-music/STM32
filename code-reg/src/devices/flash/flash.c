#include "flash.h"

static flash_t *flash_ptr = &flash_m25q128;

void flash_init(void) {
  flash_ptr->init(); //
}

void flash_write(uint32_t addr, uint8_t *buff, uint16_t size) {
  flash_ptr->write(addr, buff, size);
}

void flash_read(uint32_t addr, uint8_t *buff, uint16_t size) {
  flash_ptr->read(addr, buff, size);
}
