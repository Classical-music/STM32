#include "cmd.h"
#include "app_led.h"
#include "console.h"
#include "event.h"
#include <stdio.h>
#include <string.h>

static void exec_clear(void) {
  // \033[2J - 清楚整个屏幕
  // \033[H - 光标移动到屏幕左上角
  printf("\033[2J\033[H");
}

static void parse_cmd(char *buff, uint32_t len) {
  if (strcmp(buff, "clear") == 0) {
    exec_clear();
    return;
  } else if (strcmp(buff, "led on") == 0) {
    water_light_start();
  } else if (strcmp(buff, "led off") == 0) {
    water_light_stop();
  } else {
    printf("\n%s", buff);
  }
  printf("\n");
}

static void event_handler_cmdparse(const event_t *evt) {
  char buff[256] = {0};
  uint32_t len = CONSOLE_READ_BUFF(buff, 256);
  if (len > 0)
    parse_cmd(buff, len);
  else
    printf("\n");
  printf("hujie@ubuntu$ ");
}

void cmd_init(void) {
  console_init(); // 初始化 console
  printf("\nhujie@ubuntu$ ");

  // 注册命令解析cb
  event_register(EVT_ID_CMDLINE_READY, event_handler_cmdparse);
}
