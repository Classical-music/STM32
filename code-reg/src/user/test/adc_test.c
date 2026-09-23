#include "adc/adc.h"
#include "test.h"
#include "tim/tim.h"

// ADC 最简测试: 轮询读取内部通道, 串口打印
// 内部通道无需任何接线, 上电即可验证 ADC 是否工作
void adc_test(void) {
  adc_init();
  printf("\r\n===== ADC test =====\r\n");

  for (int i = 0; i < 10; i++) {
    // 通道 16: 内部温度传感器 (粗略值, 未做芯片个体校准)
    uint16_t raw = adc_read(ADC_CH_TEMP, ADC_SAMPLE_TIME_15CYC);
    int16_t t = adc_temp_x10(raw);
    printf("temp: raw=%4u  %d.%d C\r\n", raw, t / 10, t % 10);

    // 通道 17: 内部参考电压 VREFINT, 典型值约 1.2V
    raw = adc_read(ADC_CH_VREF, ADC_SAMPLE_TIME_15CYC);
    printf("vref: raw=%4u  %lumV\r\n", raw, (unsigned long)adc_to_mv(raw));

    // 读取外部引脚示例 (PA4 = IN4, 接电位器或外部电压):
    adc_pin_init(GPIOA, 4);
    raw = adc_read(4, ADC_SAMPLE_TIME_15CYC);
    printf("IN4 : raw=%4u  %lumV\r\n", raw, (unsigned long)adc_to_mv(raw));

    delay_ms(500);
  }
  printf("===== ADC test done =====\r\n");
}
