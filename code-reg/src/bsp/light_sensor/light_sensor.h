#ifndef __LIGHT_SENSOR_H__
#define __LIGHT_SENSOR_H__

#include "type.h"

// 环境光传感器(LDR 分压电路) 读取接口
void light_sensor_init(void);

// 滤波后的 12bit 原始值 (0-4095)
uint16_t light_sensor_read_raw(void);

// 滤波后的节点电压, 单位 mV (光照越强, 电压越低)
uint32_t light_sensor_read_mv(void);

// 节点电压 -> 光照强度百分比 0-100 (100 = 最亮), 纯换算不触发采样
uint8_t light_sensor_mv_to_percent(uint32_t mv);

// 光照强度百分比 0-100 (100 = 最亮)
uint8_t light_sensor_read_percent(void);

#endif
