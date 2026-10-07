#ifndef __ADC_H__
#define __ADC_H__

#include "type.h"

// ADC1 通道号 (STM32F103):
//   0-15  : 外部引脚 PA0-PA7(IN0-IN7), PB0-PB1(IN8-IN9), PC0-PC3(IN10-IN13)
//   16    : 内部温度传感器
//   17    : 内部参考电压 VREFINT (典型值 1.2V)
#define ADC_CH_TEMP 16
#define ADC_CH_VREF 17

// ADC3 额外通道号 (仅 STM32F103 高密度器件, 用于 ADC1 采不到的引脚):
//   4-8   : PF6-PF10 (ADC3_IN4-IN8)
//   ADC3 没有内部温度传感器与 VREFINT, 不能读 16/17
#define ADC3_CH_PF6 4
#define ADC3_CH_PF7 5
#define ADC3_CH_PF8 6
#define ADC3_CH_PF9 7
#define ADC3_CH_PF10 8

// 采样时间
// CYC : 采样时间周期数
typedef enum {
  ADC_SAMPLE_TIME_15CYC = 0, // 1.5 个时钟周期
  ADC_SAMPLE_TIME_75CYC = 1, // 7.5 个时钟周期
  ADC_SAMPLE_TIME_135CYC = 2, // 13.5 个时钟周期
  ADC_SAMPLE_TIME_285CYC = 3, // 28.5 个时钟周期
  ADC_SAMPLE_TIME_415CYC = 4, // 41.5 个时钟周期
  ADC_SAMPLE_TIME_555CYC = 5, // 55.5 个时钟周期
  ADC_SAMPLE_TIME_715CYC = 6, // 71.5 个时钟周期
  ADC_SAMPLE_TIME_2395CYC = 7, // 239.5 个时钟周期
} ADC_SampleTime;

// 初始化 ADC1: 时钟、分频、使能、校准 (默认已使能温度传感器/VREFINT)
void adc_init(void);

// 初始化 ADC3: 时钟、分频、使能、校准 (PF6-PF10 等 ADC1 不支持的引脚)
void adc3_init(void);

// 将外部 ADC 引脚配置为模拟输入 (通道 0-15 使用)
void adc_pin_init(GPIO_TypeDef *port, uint8_t pin);

// 轮询方式完成一次转换, 返回 12bit 原始值 (0-4095)
uint16_t adc_read(uint8_t channel, ADC_SampleTime sample_time);

// ADC3 轮询方式完成一次转换
uint16_t adc3_read(uint8_t channel, ADC_SampleTime sample_time);

// 原始值 -> 电压, 单位 mV (按 VDDA=3300mV 换算)
uint32_t adc_to_mv(uint16_t raw);

// 温度通道原始值 -> 温度, 单位 0.1℃ (参数取自 F103 数据手册典型值)
int16_t adc_temp_x10(uint16_t raw);

#endif
