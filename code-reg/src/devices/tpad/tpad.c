#include "tpad.h"
#include "gpio.h"
#include "rcc.h"
#include "tim.h"
#include "timer.h"

#define TPAD_ARR_MAX 0xFFFF // 定时器ARR最大值为 0xFFFF
#define TPAD_GATE_VAL 100   // 触摸阈值, 可调灵敏度

/**
 * TPAD 连接 - PA1
 * 使用 TIM5 进行输入捕获
 */

// 复位 & 放电
static void tpad_reset(void) {
  // 放电 - 配置 PA1 为推挽输出, 并输出低电平
  GPIO_MODE_SET(GPIOA, 1, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP);

  // 等待放电完成
  timer_delay_ms(5);

  // 清除捕获定时器
  tim_cap_reset();

  // 准备充电 - 配置 PA1 为浮空输入
  GPIO_MODE_SET(GPIOA, 1, GPIO_MODE_I | GPIO_CNF_I_Float);
}

// 获取单次充电时间(同步)
static uint16_t tpad_get_val(void) {
  tpad_reset(); // 先放电复位

  while (tim_cap_get_val() == 0)
    ;
  return tim_cap_get_val();
}

static void array_del_max(uint16_t buf[], int size) {
  uint16_t max = buf[0];
  int maxIdx = 0;
  for (int i = 0; i < size; i++) {
    if (buf[i] == 0)
      continue;
    if (max == 0 || buf[i] > max) {
      max = buf[i];
      maxIdx = i;
    }
  }
  buf[maxIdx] = 0;
}
static void array_del_min(uint16_t buf[], int size) {
  uint16_t min = buf[0];
  int minIdx = 0;
  for (int i = 0; i < size; i++) {
    if (buf[i] == 0)
      continue;
    if (min == 0 || buf[i] < min) {
      min = buf[i];
      minIdx = i;
    }
  }
  buf[minIdx] = 0;
}

static uint16_t tpad_get_base_val(void) {
  uint16_t buf[10];
  // 获取 10 次捕获事件
  for (int i = 0; i < 10; i++) {
    buf[i] = tpad_get_val();
  }
  // 删除最高和最低的4个
  array_del_max(buf, 10);
  array_del_max(buf, 10);
  array_del_min(buf, 10);
  array_del_min(buf, 10);
  // 计算平均值, 剩余6个数
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += buf[i];
  }
  return sum / 6;
}

static void tpad_scan_timer_cb(void *arg);
static void tpad_get_val_timer_cb(void *arg);

static uint16_t g_base_val = 0;
static timer_id_t timer_id = -1;
static bool is_scan = false;

void tpad_init(void) {
  if (timer_id != -1)
    return; // 已初始化
  // 初始化定时器
  tim_cap_init();
  // 计算默认捕获时长
  g_base_val = tpad_get_base_val();

  is_scan = false;
  // 启动循环定时器, 扫描 TPAD
  timer_id = TIMER_ADD_CB(50, tpad_scan_timer_cb, NULL, true);
}

void tpad_stop(void) {
  if (timer_id != -1) {
    timer_del(timer_id);
    timer_id = -1;
  }
}

static void tpad_scan_timer_cb(void *arg) {
  if (is_scan)
    return; // 正在扫描 tpad
  is_scan = true;
  tpad_reset(); // 先放电复位

  // 启动单发软定时器延时
  TIMER_ADD_CB(10, tpad_get_val_timer_cb, NULL, false);
}
static void tpad_get_val_timer_cb(void *arg) {
  uint16_t val = tim_cap_get_val();
  if (val > g_base_val + 100)
    printf("val: %d\n", val);
  is_scan = false; // 扫描已完成
}
