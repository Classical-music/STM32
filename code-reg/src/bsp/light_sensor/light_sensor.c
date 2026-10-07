#include "light_sensor.h"
#include "adc/adc.h"

/**
 * 环境光传感器连接 - PF8 (ADC3_IN6)
 *
 * 电路: GND -- LS1(光敏电阻) -- PF8 -- R34(47K) -- VCC3.3
 *       PF8 侧另有 R37(1K) 串联, 用于限流/隔离
 * 特性: 光照越强 -> LS1 阻值越小 -> PF8 节点电压越低 (反向关系)
 *
 * PF8 只有 ADC3 能采样, 因此本模块走 adc3_xxx 接口。
 * 上拉电阻 47K 属于高阻抗源, 采样窗口取 71.5 个 ADC 时钟
 * (9MHz 下约 7.9us), 满足源阻抗对充电时间的要求。
 */

#define LIGHT_SENSOR_Port GPIOF
#define LIGHT_SENSOR_Pin 8
#define LIGHT_SENSOR_ADC_CH ADC3_CH_PF8
#define LIGHT_SENSOR_SAMPLE_TIME ADC_SAMPLE_TIME_715CYC

// 去极值平均的采样次数: 去掉 1 个最大值和 1 个最小值后取平均
#define LIGHT_SENSOR_SAMPLES 8
#define LIGHT_SENSOR_VALID (LIGHT_SENSOR_SAMPLES - 2)

// 节点电压 -> 光照百分比的两个标定点
// 光照越强电压越低, 所以百分比 = (黑暗电压 - 当前电压) / 量程
//
// 端点由分压公式推算: V = 3300 * R_LS1 / (R_LS1 + 47000)  (R37 上无电流, 可忽略)
//   全黑: SMD201 暗阻 >= 1MΩ, V = 3300 * 1000/1047 ≈ 3152 mV, 阻值再大也只是逼近 VDD,
//         因此取 DARK = 3150
//   强光: 亮阻约 3~10KΩ, V = 199 ~ 579 mV, 取典型 5KΩ 附近的 BRIGHT = 350
// 若更换光敏管或调整 R34, 需重新推算这两个值。
#define LIGHT_MV_DARK 3150  // 接近全黑时的节点电压
#define LIGHT_MV_BRIGHT 350 // 强光下的节点电压

static uint8_t g_inited;

void light_sensor_init(void) {
  if (g_inited)
    return;

  adc3_init();
  adc_pin_init(LIGHT_SENSOR_Port, LIGHT_SENSOR_Pin);

  g_inited = 1;
}

uint16_t light_sensor_read_raw(void) {
  light_sensor_init();

  // 去掉一个最高、一个最低后求平均, 抑制尖峰干扰
  uint32_t sum = 0;
  uint16_t min = 0xFFFF;
  uint16_t max = 0;
  for (int i = 0; i < LIGHT_SENSOR_SAMPLES; i++) {
    uint16_t v = adc3_read(LIGHT_SENSOR_ADC_CH, LIGHT_SENSOR_SAMPLE_TIME);
    sum += v;
    if (v < min)
      min = v;
    if (v > max)
      max = v;
  }
  return (uint16_t)((sum - min - max) / LIGHT_SENSOR_VALID);
}

uint32_t light_sensor_read_mv(void) {
  return adc_to_mv(light_sensor_read_raw()); //
}

uint8_t light_sensor_mv_to_percent(uint32_t mv) {
  uint32_t span = LIGHT_MV_DARK - LIGHT_MV_BRIGHT;

  if (mv >= LIGHT_MV_DARK)
    return 0;
  if (mv <= LIGHT_MV_BRIGHT)
    return 100;

  return (uint8_t)((LIGHT_MV_DARK - mv) * 100 / span);
}

uint8_t light_sensor_read_percent(void) {
  return light_sensor_mv_to_percent(light_sensor_read_mv());
}
