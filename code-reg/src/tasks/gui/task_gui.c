#include "task_gui.h"
#include "lcd/lcd.h"
#include "led/led.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "task.h"
#include "views/views.h"

void vApplicationTickHook(void) {
  lv_tick_inc(1); //
}

extern void lcd_test(void);

void vGuiTask(void *arg) {
  lv_init();
  // 初始化显示驱动
  lv_port_disp_init();
  // 初始化触摸驱动
  lv_port_indev_init();

  views_init(); // 初始化视图（现在会通过 router 导航）

  while (1) {
    lv_task_handler(); // 处理 LVGL 绘制
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void taskGuiInit(void) {
  xTaskCreate(vGuiTask, "cmd", 1024, NULL, 2, NULL); //
}
