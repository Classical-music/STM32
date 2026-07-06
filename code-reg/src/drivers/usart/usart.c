#include "usart.h"
#include "cqueue.h"
#include "gpio.h"
#include "rcc.h"

// RX 缓冲区
#define RX_BUFF_SIZE 256
static uint8_t rx_buff[RX_BUFF_SIZE];
static queue_t rx_que;

// TX 缓冲区
#define TX_BUFF_SIZE 256
static uint8_t tx_buff[TX_BUFF_SIZE];
static queue_t tx_que;

void usart_init(uint32_t baud) {
  // 初始化缓冲区
  QUEUE_INIT(rx_que, rx_buff);
  QUEUE_INIT(tx_que, tx_buff);

  RCC_USART_EN(RCC_USART1);

  // 输入模式 PA9-TX 复用推挽输出50M
  GPIO_MODE_SET(GPIOA, 9, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);
  // PA10-RX 浮空输入
  GPIO_MODE_SET(GPIOA, 10, GPIO_MODE_I | GPIO_CNF_I_Float);

  // 配置 USART1
  USART1->CR1 &= ~USART_CR1_UE; // 禁用 USART
  // 配置波特率, 72Mhz
  uint32_t div = (SystemCoreClock + baud / 2) / baud;
  USART1->BRR = div;
  USART1->CR1 &= ~USART_CR1_M;    // 8bit数据位
  USART1->CR1 &= ~USART_CR1_PCE;  // 无校验
  USART1->CR2 &= ~USART_CR2_STOP; // 1个停止位

  // 使能发送器&接收器
  USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
  USART1->CR1 |= USART_CR1_RXNEIE; // 使能接收中断

  // 设置nvic
  NVIC_SetPriority(USART1_IRQn, IR_PRI_USART);
  NVIC_EnableIRQ(USART1_IRQn);

  USART1->CR1 |= USART_CR1_UE; // 使能 USART
}

void usart_send_byte(uint8_t data) {
  while (!QUEUE_PUSH(tx_que, data))
    __WFI(); // 发送缓冲区满

  USART1->CR1 |= USART_CR1_TXEIE; // 发送器空闲, 开始发送
}
static void handle_txe(void) {
  uint8_t data;
  if (QUEUE_POP(tx_que, data)) {
    USART1->DR = data; // 发送一个字节
    return;
  }
  // TX 缓冲区空, 关闭 TXE
  USART1->CR1 &= ~USART_CR1_TXEIE;
}

// rx 处理
bool usart_read_byte(uint8_t *data) {
  return QUEUE_POP(rx_que, *data); // 读rx缓冲区
}

// USART1 中断服务程序
void USART1_IRQHandler(void) {
  // rx 中断
  if ((USART1->CR1 & USART_CR1_RXNEIE) && (USART1->SR & USART_SR_RXNE)) {
    QUEUE_PUSH(rx_que, USART1->DR);
  }

  // txe 中断
  if ((USART1->CR1 & USART_CR1_TXEIE) && (USART1->SR & USART_SR_TXE)) {
    handle_txe();
  }
}
