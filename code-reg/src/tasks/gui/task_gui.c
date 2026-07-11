#include "task_gui.h"
#include "lcd/lcd.h"
#include "led/led.h"
#include "lv_port_disp.h"
#include "task.h"

void vApplicationTickHook(void) {
  lv_tick_inc(1); //
}

extern void lcd_test(void);

void vGuiTask(void *arg) {
  lv_init();
  // 初始化显示驱动
  lv_port_disp_init();

  // /* 创建测试控件：一个带标签的按钮 */
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 50, 50);
  lv_obj_set_size(btn, 120, 50);

  while (1) {
    lv_task_handler(); // 处理 LVGL 绘制
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void taskGuiInit(void) {
  xTaskCreate(vGuiTask, "cmd", 1024, NULL, 2, NULL); //
}
