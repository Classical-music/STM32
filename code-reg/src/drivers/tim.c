#include "tim.h"
#include "gpio.h"
#include "rcc.h"

// ==================== TIM3-CHY2 - PB5 PWM ====================
void tim_pwm_init(void) {
  RCC_TIM_EN(RCC_TIM3, RCC_GPIOB); // RCC

  // TIM3-CHY2 remap:10 重映射到 PB5
  AFIO->MAPR &= ~AFIO_MAPR_TIM3_REMAP;
  AFIO->MAPR |= AFIO_MAPR_TIM3_REMAP_1;

  // PB5-chy2: 复用推挽输出50M
  GPIO_MODE_SET(GPIOB, 5, GPIO_MODE_O50 | GPIO_CNF_O_Res_PP);

  // 预分频系数 10khz
  TIM3->PSC = SystemCoreClock / 10000 - 1;
  TIM3->ARR = 99; // 计数器自动重装载值 10ms

  // 捕获/比较模式寄存器
  TIM3->CCMR1 &= ~TIM_CCMR1_CC2S; // 00-输出模式
  TIM3->CCMR1 &= ~TIM_CCMR1_OC2M; // 110-pwm1模式
  TIM3->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
  TIM3->CCMR1 |= TIM_CCMR1_OC2PE; // 设置预装载使能

  TIM3->CR1 &= ~TIM_CR1_DIR; // 向上计数

  // 捕获/比较使能寄存器
  TIM3->CCER |= TIM_CCER_CC2P; // 低电平有效
  TIM3->CCER |= TIM_CCER_CC2E; // 输出使能

  // 捕获/比较寄存器2
  TIM3->CCR2 = 0; // 默认全部无效
}

void tim_pwm_start(void) {
  TIM3->CR1 |= TIM_CR1_CEN; // 使能 TIM3
}

void tim_pwm_stop(void) {
  TIM3->CR1 &= ~TIM_CR1_CEN; // 关闭 TIM3
}

void tim_pwm_dutyCycle(uint8_t pct) {
  TIM3->CCR2 = pct; // pct 0-99;
}
// ==================== TIM3-CHY2 - PB5 PWM ====================

// ==================== TIM5-CH2 - PA1 输入捕获 ====================
void tim_cap_init(void) {
  RCC_TIM_EN(RCC_TIM5, RCC_GPIOA); // RCC

  // PA1 - 浮空输入
  GPIO_MODE_SET(GPIOA, 1, GPIO_MODE_I | GPIO_CNF_I_Float);

  // 时基配置
  TIM3->PSC = SystemCoreClock / 1000000 - 1; // 1MHz
  TIM3->ARR = 0xFFFF; // 计数器,最大值,尽量在一个周期内不溢出

  // 捕获/比较XX寄存器配置
  TIM5->CCMR1 &= ~TIM_CCMR1_IC2F; // 0000-不滤波
  TIM5->CCER &= ~TIM_CCER_CC2P;   // 0-捕获上升沿

  TIM5->CCMR1 &= ~TIM_CCMR1_CC2S;
  TIM5->CCMR1 |= TIM_CCMR1_CC2S_0; // 01-输入-TI2

  TIM5->CCMR1 &= ~TIM_CCMR1_IC2PSC; // 00-不分频
  TIM5->CCER |= TIM_CCER_CC2E;      // 使能CH2捕获

  // 中断
  TIM5->DIER |= TIM_DIER_CC2IE; // 允许捕获2中断
  NVIC_SetPriority(TIM5_IRQn, IR_PRI_TIM_CAP);
  NVIC_EnableIRQ(TIM5_IRQn);

  TIM5->CR1 |= TIM_CR1_CEN; // 使能计数器
}

void tim_cap_start(void) {
  TIM5->CR1 |= TIM_CR1_CEN; // 使能计数器
}
void tim_cap_stop(void) {
  TIM5->CR1 &= ~TIM_CR1_CEN; // 关闭计数器
}

static uint16_t g_cap_val = 0; // 捕获时间, 单位us
void TIM5_IRQHandler(void) {
  if (TIM5->SR & TIM_SR_CC2IF) { // 捕获CH2中断
    TIM5->SR &= ~TIM_SR_CC2IF;
    g_cap_val = TIM5->CCR2;
  }
}

uint16_t tim_cap_get_val(void) {
  return g_cap_val; //
}

void tim_cap_reset(void) {
  TIM5->SR = 0;  // 清除中断标志
  TIM5->CNT = 0; // 计数器归零
  g_cap_val = 0;
}

// ==================== TIM5-CH1 - PA0 输入捕获 ====================
