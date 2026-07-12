#include "task_cmd.h"
#include "cmd.h"
#include "led/task_led.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static void handle_cmdparse(char *buff, size_t len) {
  exec_cmd(buff);

  printf("hujie@ubuntu$ ");
}

static void vCmdTask(void *arg) {
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
