#include "task_cmd.h"
#include "cmd.h"
#include "led/task_led.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

// static void exec_clear(void) {
//   // \033[2J - 清楚整个屏幕
//   // \033[H - 光标移动到屏幕左上角
//   printf("\033[2J\033[H");
// }

static void handle_cmdparse(char *buff, size_t len) {
  //   if (len <= 0) {
  //     goto label1;
  //   }
  //   if (strcmp(buff, "clear") == 0) {
  //     exec_clear();
  //     goto label2;
  //   } else if (strcmp(buff, "led flow") == 0) {
  //     setLedMode(LED_MODE_FLOW);
  //   } else if (strcmp(buff, "led breath") == 0) {
  //     setLedMode(LED_MODE_BREATH);
  //   } else if (strcmp(buff, "led on") == 0) {
  //     setLedMode(LED_MODE_ON);
  //   } else if (strcmp(buff, "led off") == 0) {
  //     setLedMode(LED_MODE_OFF);
  //   } else {
  //     printf("\n%s", buff);
  //   }
  exec_cmd(buff);

  printf("\nhujie@ubuntu$ ");
}

static void vCmdTask(void *arg) {
  taskConsoleInit(); //
  uint8_t recv_buff[256];
  size_t recv_len;

  while (1) {
    recv_len = xMessageBufferReceive(xConsoleBuff, recv_buff, sizeof(recv_buff),
                                     portMAX_DELAY);
    if (recv_buff[0] == '\0') {
      // messageBuffer 无法接受长度为0的消息, 因此需要做判断
      recv_len = 0;
    } else {
      // messageBuffer 不会自动添加 '\0'
      recv_buff[recv_len] = '\0';
    }
    handle_cmdparse((char *)recv_buff, recv_len);
  }
}

void cmdTaskInit(void) {
  xTaskCreate(vCmdTask, "cmd", 128, NULL, 2, NULL); //
}
