#ifndef __SPI_H__
#define __SPI_H__

#include "type.h"

// 初始化
void spi_init(void);
// 读写字节
uint8_t spi_rw(uint8_t data);

#endif
