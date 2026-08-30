#include "bsp_touch.h"
#include "gpio.h"
#include "i2c/i2c.h"
#include "lcd/lcd.h"
#include "tim/tim.h"
#include <stdio.h>
#include <string.h>

static void gt911_i2c_write(uint16_t addr, uint8_t *buff, uint8_t len);
static void gt911_i2c_read(uint16_t addr, uint8_t *buff, uint8_t len);

/**
 * 引脚
 * INT - T_PEN - PF10 - 浮空输入
 */

// ============================== 上电启动 ==============================
// RST - T_CS - PF11 - 通用推挽输出
// INT - T_PEN - PF10 - 下拉输出 -> 浮空输入

#define W_TOUCH_ADDR 0xBA
#define R_TOUCH_ADDR 0xBB

static void gt911_start(void) {
  // RCC
  RCC->APB2ENR |= RCC_APB2ENR_IOPFEN;
  // gpio
  GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Gen_PP, 10, 11);
  // 拉低 RST & INT
  GPIO_LOW(GPIOF, 11); // RST
  GPIO_LOW(GPIOF, 10); // INT
  delay_ms(10);
  // 拉高 RST
  GPIO_HIGH(GPIOF, 11); // RST
  delay_ms(100);
  // INT 切换为浮空输入, 下拉, 检测上升沿
  GPIO_SET_MODES(GPIOF, GPIO_MODE_I | GPIO_CNF_I_Float, 10);
  GPIO_LOW(GPIOF, 10);
}

// ============================== INT ==============================
// INT - T_PEN - PF10 - 浮空输入 + 中断检测
static void INT_Init_IR(void) {
  // RCC
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

  // 映射 EXTI 中断到 PF10
  AFIO->EXTICR[2] &= ~AFIO_EXTICR3_EXTI10;
  AFIO->EXTICR[2] |= AFIO_EXTICR3_EXTI10_PF;

  // 中断
  EXTI->PR |= EXTI_PR_PR10;     // 清除可能得挂起标志
  EXTI->RTSR |= EXTI_RTSR_TR10; // 上升沿触发
  EXTI->IMR |= EXTI_IMR_MR10;   // 使能中断

  // NVIC
  NVIC_SetPriority(EXTI15_10_IRQn, IR_PRI_TOUCH);
  NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static volatile bool gt911_touched = false;
static volatile uint16_t gt911_x = 0;
static volatile uint16_t gt911_y = 0;
static touch_irq_callback_t gt911_irq_callback = NULL;

void EXTI15_10_IRQHandler(void) {
  if (EXTI->PR & EXTI_PR_PR10) {
    EXTI->PR = EXTI_PR_PR10; // 清除硬件中断标志
    if (gt911_irq_callback) // 调用中断回调, 唤醒 Touch Task
      gt911_irq_callback();
  }
}
// 中断回调注册函数
static void gt911_set_irq_callback(touch_irq_callback_t callback) {
  gt911_irq_callback = callback;
}
// touch 任务处理函数
static void gt911_process(void) {
  uint8_t status = 0;
  // 先读取 0x814E 判断是否有中断
  gt911_i2c_read(0x814E, &status, 1);
  if (!(status & 0x80))
    return; // 没有中断, 直接返回

  uint8_t touch_num = status & 0x0F;
  if (touch_num > 0 && touch_num <= 5) {
    // 读取第一个触摸点坐标，0x8150和0x8152
    uint8_t buf[6];
    gt911_i2c_read(0x8150, buf, 6);
    // 组合坐标 (低字节在前)
    gt911_x = ((uint16_t)buf[1] << 8) | buf[0];
    gt911_y = ((uint16_t)buf[3] << 8) | buf[2];
    gt911_touched = true;
  } else {
    gt911_touched = false;
  }

  // 清除中断标志（写0到0x814E）
  uint8_t clear = 0;
  gt911_i2c_write(0x814E, &clear, 1);
}

// 判断是否按下
static bool gt911_is_pressed(void) {
  return gt911_touched; //
}

// 获取坐标
static void gt911_get_xy(int32_t *x, int32_t *y) {
  *x = gt911_x;
  *y = gt911_y;
}

// ============================== I2C ==============================
/**
 * SCL - T_SLK - PB1 - 通用开漏输出
 * SDA - T_MOSI - PF9 - 通用开漏输出
 */
static i2c_ctx_t i2c_ctx = {
    .scl_port = GPIOB,
    .scl_pin = 1,
    .sda_port = GPIOF,
    .sda_pin = 9,
};

static void gt911_i2c_init(void) {
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPFEN;

  // gpio 模式
  GPIO_SET_MODES(GPIOB, GPIO_MODE_O50 | GPIO_CNF_O_Gen_OD, 1);
  GPIO_SET_MODES(GPIOF, GPIO_MODE_O50 | GPIO_CNF_O_Gen_OD, 9);

  // 初始化 I2C
  I2C_INIT(i2c_ctx);
}

static void gt911_i2c_read(uint16_t addr, uint8_t *buff, uint8_t len) {
  // 发送起始信息
  if (!I2C_START(i2c_ctx)) {
    printf("%s:%d i2c_start failed\n", __FILE__, __LINE__);
    return;
  }

  // 发送设备地址(写)
  if (!I2C_SEND_ADDR(i2c_ctx, W_TOUCH_ADDR)) {
    printf("%s:%d\n", __FILE__, __LINE__);
    I2C_STOP(i2c_ctx);
    return;
  }

  // 发送内部地址H
  if (!I2C_SEND_BYTE(i2c_ctx, addr >> 8)) {
    printf("%s:%d\n", __FILE__, __LINE__);
    I2C_STOP(i2c_ctx);
    return;
  }
  // 发送内部地址L
  if (!I2C_SEND_BYTE(i2c_ctx, addr & 0xFF)) {
    printf("%s:%d\n", __FILE__, __LINE__);
    I2C_STOP(i2c_ctx);
    return;
  }

  // 重新发送起始信号
  if (!I2C_START(i2c_ctx)) {
    printf("%s:%d\n", __FILE__, __LINE__);
    I2C_STOP(i2c_ctx);
    return;
  }

  // 发送设备地址(读)
  if (!I2C_SEND_ADDR(i2c_ctx, R_TOUCH_ADDR)) {
    printf("%s:%d\n", __FILE__, __LINE__);
    I2C_STOP(i2c_ctx);
    return;
  }

  // 读取数据
  for (uint16_t i = 0; i < len; i++) {
    buff[i] = I2C_READ_BYTE(i2c_ctx, i < len - 1);
  }

  // 发送结束信号
  I2C_STOP(i2c_ctx);
}

static void gt911_i2c_write(uint16_t addr, uint8_t *buff, uint8_t len) {
  // 发出 start 信号
  if (!I2C_START(i2c_ctx)) {
    return;
  }

  // 发送写地址
  if (!I2C_SEND_ADDR(i2c_ctx, W_TOUCH_ADDR)) {
    I2C_STOP(i2c_ctx);
    return;
  }

  // 发送内部地址H
  if (!I2C_SEND_BYTE(i2c_ctx, addr >> 8)) {
    I2C_STOP(i2c_ctx);
    return;
  }
  // 发送内部地址L
  if (!I2C_SEND_BYTE(i2c_ctx, addr & 0xFF)) {
    I2C_STOP(i2c_ctx);
    return;
  }

  for (uint8_t i = 0; i < len; i++) {
    if (!I2C_SEND_BYTE(i2c_ctx, buff[i])) {
      break;
    }
  }

  // 发送停止信号
  I2C_STOP(i2c_ctx);
}

// ============================== 芯片功能 ==============================
static bool gt911_check_id(void) {
  uint8_t buff[4] = {0};
  gt911_i2c_read(0x8140, buff, 4); //
  if (strcmp((const char *)buff, "911") != 0) {
    printf("touchId: %s %x\n", buff, buff[3]);
    return false;
  }

  return true;
}

static bool gt911_init(void) {
  gt911_start(); // 上电复位

  gt911_i2c_init();
  if (!gt911_check_id())
    return false;

  uint8_t temp = 0x02;
  gt911_i2c_write(0x8040, &temp, 1); // 软复位
  delay_ms(10);
  temp = 0x00;
  gt911_i2c_write(0x8040, &temp, 1); // 软复位结束, 进入读坐标状态

  gt911_i2c_read(0x804D, &temp, 1);
  printf("0x804D = %x\n", temp);

  INT_Init_IR(); // INT 初始化为中断方式

  return true;
}

touch_t touch = {
    .init = gt911_init,
    .is_pressed = gt911_is_pressed,
    .get_xy = gt911_get_xy,
    .set_irq_callback = gt911_set_irq_callback,
    .process = gt911_process,
};
