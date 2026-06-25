#include "beep.h"

// BEEP - PB8, 高亮-低灭
#define BEEP_Port GPIOB
#define BEEP_Pin 8

void beep_init(void) {
  // PB8 - 通用推挽输出
  GPIO_MODE_SET(BEEP_Port, BEEP_Pin, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP);
  beep_off();
}

void beep_on(void) {
  // 高电平, 开启
  GPIO_HIGH(BEEP_Port, BEEP_Pin);
}
void beep_off(void) {
  // 低电平, 关闭
  GPIO_LOW(BEEP_Port, BEEP_Pin);
}
void beep_toggle(void) {
  // 切换电平
  GPIO_TOGGLE(BEEP_Port, BEEP_Pin);
}
