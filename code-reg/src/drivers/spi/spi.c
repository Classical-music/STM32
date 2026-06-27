#include "spi.h"

void spi_init(void) {
  // 禁用 spi
  SPI2->CR1 &= ~SPI_CR1_SPE;

  SPI2->CR1 &= ~SPI_CR1_DFF;    // 8bit 数据帧
  SPI2->CR1 &= ~SPI_CR1_RXONLY; // 全双工

  SPI2->CR1 |= SPI_CR1_SSM; // 软件片选
  SPI2->CR1 |= SPI_CR1_SSI; // 禁止软件从设备, 即做主设备

  SPI2->CR1 &= ~SPI_CR1_LSBFIRST; // MSB优先

  SPI2->CR1 &= ~SPI_CR1_BR; // 波特率2分频

  SPI2->CR1 |= SPI_CR1_MSTR; // 主模式

  SPI2->CR1 &= ~SPI_CR1_CPOL; // 相位&极性
  SPI2->CR1 &= ~SPI_CR1_CPHA;

  // 启用spi
  SPI2->CR1 |= SPI_CR1_SPE;

  spi_rw(0xff); // 启动传输, 用于清空DR
}

uint8_t spi_rw(uint8_t data) {
  while (!(SPI2->SR & SPI_SR_TXE))
    ; // 等待发送缓冲区空

  SPI2->DR = data;

  while (!(SPI2->SR & SPI_SR_RXNE))
    ; // 等待接收缓冲区非空

  return SPI2->DR;
}
