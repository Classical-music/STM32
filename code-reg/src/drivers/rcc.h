#ifndef __RCC_H__
#define __RCC_H__

#include "type.h"

enum {
  RCC_GPIOA = RCC_APB2ENR_IOPAEN,
  RCC_GPIOB = RCC_APB2ENR_IOPBEN,
  RCC_GPIOD = RCC_APB2ENR_IOPDEN,
  RCC_GPIOE = RCC_APB2ENR_IOPEEN,
  RCC_GPIOG = RCC_APB2ENR_IOPGEN,

  RCC_USART1 = RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN,

  RCC_WWDG = RCC_APB1ENR_WWDGEN,

  RCC_TIM3 = RCC_APB1ENR_TIM3EN,
  RCC_TIM5 = RCC_APB1ENR_TIM5EN,
};

#define RCC_GPIO_EN(port) RCC->APB2ENR |= port

#define RCC_USART_EN(port) RCC->APB2ENR |= port

#define RCC_WWDG_EN(port) RCC->APB1ENR |= port

#define RCC_TIM_EN(tim, gpio)                                                  \
  RCC_GPIO_EN(gpio | RCC_APB2ENR_AFIOEN);                                      \
  RCC->APB1ENR |= tim

#endif
