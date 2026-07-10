#include "console/task_console.h"
#include "beep/task_beep.h"
#include "board.h"
#include "task.h"
#include "test/test.h"

#define LINE_BUFF_SIZE 256
static char line_buff[LINE_BUFF_SIZE];
static uint16_t line_len = 0;

MessageBufferHandle_t xConsoleBuff = NULL;

static void console_process_byte(uint8_t ch) {
  switch (ch) {
  case '\r': // 回车
    line_buff[line_len] = '\0';
    // 通知命令解析模块
    if (line_len == 0) {
      line_len = 1; // messageBuffer 无法接受长度为0的消息
    }
    xMessageBufferSend(xConsoleBuff, line_buff, line_len, portMAX_DELAY);
    line_len = 0; // 行缓冲清零
    break;

  case '\b': // 退格
    if (line_len > 0) {
      line_len--;
      printf("\b \b");
    } else {
      playAlert();
    }
    break;

  default:
    if (line_len < LINE_BUFF_SIZE - 1) {
      usart_send_byte(ch);        // 回显
      line_buff[line_len++] = ch; // 存入缓冲区
    } else {
      // 缓冲区满, ALERT
      playAlert();
    }
    break;
  }
}

static void console_poll_rx() {
  uint8_t ch;
  while (usart_read_byte(&ch)) {
    console_process_byte(ch);
  }
}

static void vConsoleTask(void *arg) {
  // 初始化 usart 硬件
  usart_init(115200);

  // 初始化 board
  board_init();

  test();

  TickType_t xLasWakeTime = xTaskGetTickCount();

  while (1) {
    // 10ms 处理一次 rx
    vTaskDelayUntil(&xLasWakeTime, pdMS_TO_TICKS(10));

    console_poll_rx();
  }
}

void taskConsoleInit(void) {
  // 初始化队列, 需要加上4B开销
  xConsoleBuff = xMessageBufferCreate(LINE_BUFF_SIZE + 4);
  xTaskCreate(vConsoleTask, "console", 128, NULL, 2, NULL);
}
