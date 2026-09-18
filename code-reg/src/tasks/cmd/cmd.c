#include "cmd.h"
#include "led/task_led.h"
#include "rtc/task_rtc.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// 前向声明
static void cmd_clear(char *);
static void cmd_led(char *);
static void cmd_time(char *);

// 命令表结构
typedef struct {
  const char *name; // 命令名称
  void (*handler)(char *arg);
} cmd_t;

// 预定义命令列表
static const cmd_t cmds[] = {
    {"clear", cmd_clear},
    {"led", cmd_led},
    {"time", cmd_time},
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

// time                    打印当前时间
// time YYYY-MM-DD HH:MM:SS  校时(本地时间, 东八区)
static void cmd_time(char *arg) {
  char buf[24];

  if (arg == NULL || *arg == 0) {
    printf("\n当前时间: %s", rtcFormatNow(buf, sizeof(buf)));
    if (!rtcIsReady()) {
      printf(" (RTC 未就绪, 时间不走时)");
    }
    printf("\nepoch: %lu\n", (unsigned long)rtcGetEpoch());
    return;
  }

  rtc_time_t t;
  unsigned int y, mo, d, h, mi, s;
  if (sscanf(arg, "%u-%u-%u %u:%u:%u", &y, &mo, &d, &h, &mi, &s) != 6) {
    printf("\n用法: time YYYY-MM-DD HH:MM:SS\n");
    return;
  }
  t.year = (uint16_t)y;
  t.month = (uint8_t)mo;
  t.day = (uint8_t)d;
  t.hour = (uint8_t)h;
  t.minute = (uint8_t)mi;
  t.second = (uint8_t)s;

  if (!rtcSetLocalTime(&t)) {
    printf("\n校时失败: 时间无效或 RTC 未就绪\n");
    return;
  }
  printf("\n校时成功: %s\n", rtcFormatNow(buf, sizeof(buf)));
}
