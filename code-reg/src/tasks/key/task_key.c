#include "task_key.h"
#include "key/key.h"
#include "queue.h"
#include "task.h"

// 时间参数
#define DEBOUNCE_TICK 2     // 20ms
#define LONG_PRESS_TICK 100 // 1000ms
#define HOLD_TICK 20        // 200ms

// 按键状态枚举
typedef enum {
  ST_IDLE,         // 空闲
  ST_DEB_PRESS,    // 按下, 防抖中
  ST_PRESSED,      // 已确认短按
  ST_LONG_PRESSED, // 已确认长按
  ST_DEB_RELEASE,  // 释放, 防抖中
} KeyState_t;

// 按键上下文
typedef struct {
  KeyState_t state;
  uint8_t deb_cnt;   // 防抖计数器 0~2
  uint8_t press_cnt; // 按下持续时间
  uint8_t hold_cnt;  // hold 累计
  bool is_long_mode; // 标记是否已进入长按
} KeyCtx_t;

#define KEY_TOTAL (KEY_NUM + 1) // 4*key + 1*tpad
static KeyCtx_t key_ctx[KEY_TOTAL];

QueueHandle_t xKeyQue = NULL; // 按键事件队列

// 判断按键是否按下
static bool keyIsActive(uint8_t id) {
  if (id < KEY_NUM) {
    return key_ispress(id);
  } else {
    return false;
  }
}

static void keyScan(void) {
  for (uint8_t id = 0; id < KEY_TOTAL; id++) {
    KeyCtx_t *ctx = &key_ctx[id];
    bool active = keyIsActive(id);

    switch (ctx->state) {
    case ST_IDLE:
      if (active) {
        // 进入按下防抖中状态
        ctx->state = ST_DEB_PRESS;
        ctx->deb_cnt = DEBOUNCE_TICK;
      }
      break;
    case ST_DEB_PRESS:
      if (active) {
        ctx->deb_cnt--;
        if (ctx->deb_cnt == 0) {
          // 防抖成功, 触发 Pressed 事件
          QUEUE_KEY_SEND(id, KEY_EVT_PRESSED);
          ctx->state = ST_PRESSED;
          ctx->press_cnt = LONG_PRESS_TICK;
          ctx->is_long_mode = false;
        }
      } else {
        // 抖动
        ctx->state = ST_IDLE;
      }
      break;

    case ST_PRESSED:
      if (active) {
        ctx->press_cnt--;
        if (ctx->press_cnt == 0) {
          // 长按达到1000ms, 触发 LONG_PRESSED
          QUEUE_KEY_SEND(id, KEY_EVT_LONG_PRESSED);
          ctx->state = ST_LONG_PRESSED;
          ctx->hold_cnt = HOLD_TICK;
          ctx->is_long_mode = true;
        }
      } else {
        // 检测到松开, 进入释放防抖
        ctx->state = ST_DEB_RELEASE;
        ctx->deb_cnt = DEBOUNCE_TICK;
      }
      break;

    case ST_LONG_PRESSED:
      if (active) {
        ctx->hold_cnt--;
        if (ctx->hold_cnt == 0) {
          // 触发hold事件
          QUEUE_KEY_SEND(id, KEY_EVT_HOLD);
          ctx->hold_cnt = HOLD_TICK;
        }
      } else {
        // 检测到松开, 进入释放防抖
        ctx->state = ST_DEB_RELEASE;
        ctx->deb_cnt = DEBOUNCE_TICK;
      }
      break;

    case ST_DEB_RELEASE:
      if (active) {
        // 释放抖动, 状态还原
        if (ctx->is_long_mode) {
          ctx->state = ST_LONG_PRESSED;
          ctx->hold_cnt = HOLD_TICK; // 重置 hold, 避免立即触发
        } else {
          ctx->state = ST_PRESSED;
          // press_ctn 不变, 计时不会因抖动而损失
        }
      } else {
        // 成功 release
        ctx->deb_cnt--;
        if (ctx->deb_cnt == 0) {
          if (ctx->is_long_mode) {
            // 触发 long_release 事件
            QUEUE_KEY_SEND(id, KEY_EVT_LONG_RELEASED);
          } else {
            // 触发 release 事件
            QUEUE_KEY_SEND(id, KEY_EVT_RELEASED);
          }
          ctx->state = ST_IDLE;
          ctx->is_long_mode = false;
        }
      }

    default:
      break;
    }
  }
}

static void vKeyScanTask(void *arg) {
  // 初始化 key 驱动
  key_init();

  // 基准时延
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1) {
    // 每10ms 扫描一次
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

    // 扫描所有按键
    keyScan();
  }
}

void keyTaskInit(void) {
  // 创建按键事件队列
  xKeyQue = xQueueCreate(2, sizeof(KeyMsg_t));
  // 创建按键&tpad扫描任务
  xTaskCreate(vKeyScanTask, "keyScanTask", 128, NULL, 3, NULL);
}
