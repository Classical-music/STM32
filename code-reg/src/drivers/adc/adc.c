#include "adc.h"
#include "gpio.h"
#include "rcc.h"
#include "tim/tim.h"

// ==================== ADC1 轮询单次转换 ====================
// 最简单的 ADC 用法: 软件触发 -> 等待 EOC -> 读 DR
// 转换流程: 采样时间 + 12.5 周期转换时间 (ADC 时钟 9MHz 时约 20us)

void adc_init(void) {
  // 1. 使能 ADC1 时钟 (APB2)
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

  // 2. ADC 时钟 = PCLK2 / 8 (72MHz / 8 = 9MHz: 普通通道上限 14MHz,
  //    读内部温度传感器时手册要求 ADCCLK 不超过 9MHz)
  RCC->CFGR &= ~RCC_CFGR_ADCPRE;
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8;

  // 3. 软件触发三件套: EXTSEL[2:0]=111 + EXTTRIG=1 + SWSTART
  ADC1->CR2 |= ADC_CR2_EXTSEL;
  ADC1->CR2 |= ADC_CR2_EXTTRIG;

  // 4. 使能内部温度传感器与 VREFINT (读通道 16/17 需要)
  ADC1->CR2 |= ADC_CR2_TSVREFE;

  // 5. 上电: 置 ADON 位, 等待稳定
  ADC1->CR2 |= ADC_CR2_ADON;
  delay_ms(1);

  // 6. 校准: 先复位校准, 再启动校准, 等待校准完成
  ADC1->CR2 |= ADC_CR2_RSTCAL;
  while (ADC1->CR2 & ADC_CR2_RSTCAL)
    ;
  ADC1->CR2 |= ADC_CR2_CAL;
  while (ADC1->CR2 & ADC_CR2_CAL)
    ;
}

void adc_pin_init(GPIO_TypeDef *port, uint8_t pin) {
  // 外部通道引脚必须配置为模拟输入 (CNF=00, MODE=00)
  if (port == GPIOA)
    RCC_GPIO_EN(RCC_GPIOA);
  else if (port == GPIOB)
    RCC_GPIO_EN(RCC_GPIOB);
  else if (port == GPIOC)
    RCC_GPIO_EN(RCC_GPIOC);
  GPIO_MODE_SET(port, pin, GPIO_MODE_I | GPIO_CNF_I_Ana);
}

uint16_t adc_read(uint8_t channel, ADC_SampleTime sample_time) {
  // 1. 采样时间 通道 0-9  在 SMPR2, 通道 10-17 在 SMPR1, 每个通道占 3bit
  volatile uint32_t *smpr =
      (channel < 10) ? &ADC1->SMPR2 : &ADC1->SMPR1;
  uint8_t shift = (channel < 10) ? (channel * 3) : ((channel - 10) * 3);
  *smpr &= ~(0x7 << shift);
  *smpr |= (sample_time << shift);

  // 2. 规则序列: 只转换 1 次 (SQR1 的 L[3:0]=0), 序列长度默认为 0, 无需配置

  // 3. 选择通道: 写入 SQR3 的 SQ1[4:0]
  ADC1->SQR3 = channel & 0x1F;

  // 4. 软件触发: 清除 EOC 标志后置 SWSTART
  (void)ADC1->DR; // 读 DR 清除 EOC 标志
  ADC1->CR2 |= ADC_CR2_SWSTART;

  // 5. 等待转换完成, 读取 DR (右对齐)
  while (!(ADC1->SR & ADC_SR_EOC))
    ;
  return (uint16_t)(ADC1->DR & 0xFFFF);
}

// 原始值 -> mV, 按 VDDA = 3300mV 计算
uint32_t adc_to_mv(uint16_t raw) { return (uint32_t)raw * 3300 / 4096; }

// 温度通道原始值 -> 温度, 单位 0.1°C
// F103 数据手册: 25°C 时 V25 = 1.43V, 斜率约 4.3mV/°C
int16_t adc_temp_x10(uint16_t raw) {
  int32_t mv = (int32_t)adc_to_mv(raw);
  return (int16_t)((mv - 1430) * 100 / 43 + 250);
}
