#include "board.h"
#include "beep/beep.h"
#include "eeprom/eeprom.h"
#include "flash/flash.h"
#include "key/key.h"
#include "lcd/lcd.h"
#include "led/led.h"
#include "light_sensor/light_sensor.h"
#include "rtc/bsp_rtc.h"
#include "touch/bsp_touch.h"
#include "tpad/tpad.h"
#include "usart/usart.h"
#include "wwdg/wwdg.h"

void board_init(void) {
  // 首选初始化 usart, 提供printf能力
  usart_init(115200);
  // 其次初始化 led&beep, 用于调试
  led_init();
  beep_init();

  key_init();  // 初始化 key 驱动
  tpad_init(); // 初始化 tpad 驱动

  light_sensor_init(); // 初始化环境光传感器(PF8/ADC3)

  bsp_rtc_init(); // 初始化 RTC 秒计数器(备份域魔数保证复位后继续走时)

  // wwdg_init(0x7F, 0x5F, WWDG_WDGTB_8); // 初始化看门狗

  // 初始化存储, 提供资源
  flash_init();    // 初始化 Flash
  eeprom_init();   // 初始化 EEPROM

  lcd_init();   // 初始化 LCD
  touch_init(); // 初始化触摸屏
}
