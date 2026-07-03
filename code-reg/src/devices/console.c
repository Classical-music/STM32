#include "console.h"
// #include "app_beep.h"
#include "event.h"
#include "timer.h"
#include <stdio.h>
#include <string.h>

#define LINE_BUFF_SIZE 256
static char line_buff[LINE_BUFF_SIZE];
static uint16_t line_len = 0;

static void console_process_byte(uint8_t ch) {
  switch (ch) {
  case '\r': { // 回车
    line_buff[line_len] = '\0';
    // 发送外部事件: 命令行解析
    event_t evt = {EVT_ID_CMDLINE_READY};
    event_push(&evt);
    break;
  }
  case '\b': // 退格
    if (line_len > 0) {
      line_len--;
      printf("\b \b");
    } else {
      //   playAlert();
    }
    break;

  default:
    if (line_len < LINE_BUFF_SIZE - 1) {
      usart_send_byte(ch);        // 回显
      line_buff[line_len++] = ch; // 存入缓冲区
    } else {
      // 缓冲区满&丢弃&ALERT
      //   playAlert();
    }
    break;
  }
}

static void console_poll_timer_cb(void *arg) {
  uint8_t ch;
  while (usart_read_byte(&ch)) {
    console_process_byte(ch);
  }
}

void console_init(void) {
  usart_init(115200); // 初始化 usart

  // 周期定时器, 处理 rx
  TIMER_ADD_CB(10, console_poll_timer_cb, NULL, true);
}

uint32_t console_read_buff(uint8_t *buff, uint32_t size) {
  if (size > line_len) {
    size = line_len;
  }
  memcpy(buff, line_buff, size); // 拷贝数据
  line_len = 0;                  // 清空buff
  buff[size] = '\0';
  return size;
}

int fputc(int ch, FILE *f) {
  if (ch == '\n') {
    usart_send_byte('\r');
  }
  usart_send_byte(ch);
  return ch;
}
