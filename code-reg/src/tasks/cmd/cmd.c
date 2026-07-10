#include "cmd.h"
#include "led/task_led.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// 前向声明
void cmd_clear(char *);
void cmd_led(char *);

// 命令表结构
typedef struct {
  const char *name; // 命令名称
  void (*handler)(char *arg);
} cmd_t;

// 预定义命令列表
static const cmd_t cmds[] = {
    {"clear", cmd_clear},
    {"led", cmd_led},
};

// 去除首位空字符
char *trim(char *str) {
  char *end;
  // 去除开头空格
  while (isspace(*str))
    str++;
  if (*str == 0)
    return str;

  // end 到达尾部
  end = str;
  while (*end)
    end++;
  end--;
  // 去除尾部空格
  while (end > str && isspace(*end))
    end--;
  end[1] = 0;
  return str;
}

// 查找命令
const cmd_t *find_cmd(const char *name) {
  for (uint8_t i = 0; i < ARRAY_SIZE(cmds); i++) {
    const cmd_t *cmd = &cmds[i];
    if (strcmp(cmd->name, name) == 0) {
      return cmd;
    }
  }
  return NULL;
}

void exec_cmd(char *cmdstr) {
  // 去除首尾空格, 第一段是name
  char *name = trim(cmdstr);
  if (*name == 0) {
    printf("\n");
    return; // 空行忽略
  }

  // 第二段是参数
  char *args = strpbrk(name, " \t\r\n");
  if (args) {
    *args = 0; // 截断 name
    args = trim(args + 1);
  }

  const cmd_t *cmd = find_cmd(name);
  if (cmd == NULL) {
    printf("\n未知命令: '%s'\n", name);
  } else {
    cmd->handler(args);
  }
}

static void cmd_clear(char *arg) {
  // \033[2J - 清楚整个屏幕
  // \033[H - 光标移动到屏幕左上角
  printf("\033[2J\033[H");
}

static void cmd_led(char *arg) {
  if (strcmp(arg, "flow") == 0) {
    setLedMode(LED_MODE_FLOW);
  } else if (strcmp(arg, "breath") == 0) {
    setLedMode(LED_MODE_BREATH);
  } else if (strcmp(arg, "on") == 0) {
    setLedMode(LED_MODE_ON);
  } else if (strcmp(arg, "off") == 0) {
    setLedMode(LED_MODE_OFF);
  } else {
    printf("\nled 未知参数: '%s'", arg);
  }
  printf("\n");
}
