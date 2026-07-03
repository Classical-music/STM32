#include "key_fsm.h"
#include "fsm.h"
#include "timer.h"

// 按键防抖状态机
enum {
  KEY_ST_IDLE,    // 未按下
  KEY_ST_DEB,     // 消抖等待
  KEY_ST_PRESSED, // 按下成功PA
  KEY_ST_LPRESSD, // 长按状态
};

// 状态机内部事件
enum {
  KEY_PRESS_DET,
  KEY_DEB_TIMEOUT,
  KEY_RELEASE_DET,
  KEY_LPRESS_TIMEOUT,
  KEY_HOLD_TIMEOUT,
};

// 按键上下文
typedef struct {
  key_id_t id;
  fsm_t fsm;
} key_ctx_t;

static key_ctx_t keys[KEY_NUM];

// 函数声明
static void key_scan_timer_cb(void *arg);
static void key_deb_timeout_cb(void *arg);
static void key_long_timeout_cb(void *arg);
static void key_hold_timeout_cb(void *arg);
// 动作函数
static state_t act_press(fsm_t *fsm, const event_t *ev);
static state_t act_debounce(fsm_t *fsm, const event_t *ev);
static state_t act_release(fsm_t *fsm, const event_t *ev);
static state_t act_lpress(fsm_t *fsm, const event_t *ev);
static state_t act_lrelease(fsm_t *fsm, const event_t *ev);
static state_t act_hold(fsm_t *fsm, const event_t *ev);

#define GET_CTX(fsm) container_of(fsm, key_ctx_t, fsm)

// 状态机转换表
static const transition_t key_transitions[] = {
    {KEY_ST_IDLE, KEY_PRESS_DET, act_press, KEY_ST_DEB},
    {KEY_ST_DEB, KEY_DEB_TIMEOUT, act_debounce, KEY_ST_PRESSED},
    {KEY_ST_PRESSED, KEY_RELEASE_DET, act_release, KEY_ST_IDLE},
    {KEY_ST_PRESSED, KEY_LPRESS_TIMEOUT, act_lpress, KEY_ST_LPRESSD},
    {KEY_ST_LPRESSD, KEY_RELEASE_DET, act_lrelease, KEY_ST_IDLE},
    {KEY_ST_LPRESSD, KEY_HOLD_TIMEOUT, act_hold, KEY_ST_LPRESSD},
};

// 定时器id
timer_id_t timer_id_lpress = -1;
timer_id_t timer_id_hold = -1;

void key_fsm_init(void) {
  // 初始化 每个按键 状态机
  for (int i = 0; i < KEY_NUM; i++) {
    keys[i].id = (key_id_t)i;
    FSM_INIT(keys[i].fsm, KEY_ST_IDLE, key_transitions);
  }

  // 启动周期扫描定时器(10ms一次)
  timer_add_cb(10, key_scan_timer_cb, NULL, true);
}

// ==================== 定时器回调, 产生内部定时器事件 ====================
// 扫描 cb, 检测电平变化, -> PRESS-DET or RELEASE-DET 事件
static void key_scan_timer_cb(void *arg) {
  // 初始时, 所有按钮未按下
  static bool last_press_state[] = {false, false, false, false};

  for (key_id_t id = KEY_START; id < KEY_NUM; id++) {
    bool is_press = key_ispress(id); // 检测当前按键状态
    if (is_press == last_press_state[id])
      continue; // 按下状态未变化
    event_t evt = {
        .type = is_press ? KEY_PRESS_DET : KEY_RELEASE_DET,
        .param = id, // 携带 key_id
    };
    FSM_DISPATCH(keys[id].fsm, &evt); // 事件发送给状态机
    last_press_state[id] = is_press;
  }
}
// deb 超时 -> DEB-TIMEOUT 事件
void key_deb_timeout_cb(void *arg) {
  key_ctx_t *ctx = (key_ctx_t *)arg;
  event_t evt = {KEY_DEB_TIMEOUT, ctx->id};
  FSM_DISPATCH(ctx->fsm, &evt);
}
// lpress 超时 -> LPRESS-TIMEOUT 事件
void key_long_timeout_cb(void *arg) {
  key_ctx_t *ctx = (key_ctx_t *)arg;
  event_t evt = {KEY_LPRESS_TIMEOUT, ctx->id};
  FSM_DISPATCH(ctx->fsm, &evt);
}
// hold 超时 -> HOLD-TIMEOUT 事件
void key_hold_timeout_cb(void *arg) {
  key_ctx_t *ctx = (key_ctx_t *)arg;
  event_t evt = {KEY_HOLD_TIMEOUT, ctx->id};
  FSM_DISPATCH(ctx->fsm, &evt);
}

// ==================== act 回调, 处理事件, 切换状态 ====================
// ST_IDLE & PRESS-DET => ST_DEB
static state_t act_press(fsm_t *fsm, const event_t *evt) {
  key_ctx_t *ctx = GET_CTX(fsm);
  // 启动消陡定时器 20ms & 单次 & cb
  TIMER_ADD_CB(20, key_deb_timeout_cb, ctx, false);
  return KEY_ST_DEB;
}
// ST_DEB & DEB_TIMEOUT => ST_PRESSED
static state_t act_debounce(fsm_t *fsm, const event_t *evt) {
  key_ctx_t *ctx = GET_CTX(fsm);
  bool is_press = key_ispress(ctx->id);
  if (!is_press)
    return KEY_ST_IDLE; // 抖动, 回到空闲

  // 启动长按检测定时器 1000ms & 单次 & cb
  timer_id_lpress = TIMER_ADD_CB(1000, key_long_timeout_cb, ctx, false);
  return KEY_ST_PRESSED; // 进入按下状态
}

// ST_PRESSED & LPRESS_TIMEOUT -> ST_LPRESSD
static state_t act_lpress(fsm_t *fsm, const event_t *evt) {
  key_ctx_t *ctx = GET_CTX(fsm);
  // 发送按键长按事件
  event_t evt2 = {EVT_ID_KEY_LPRESSED, ctx->id};
  event_push(&evt2);
  // 启动hold定时器: 200ms & 周期 & 回调
  timer_id_hold = TIMER_ADD_CB(200, key_hold_timeout_cb, ctx, true);
  return KEY_ST_LPRESSD; // 进入长按事件
}

// ST_LPRESSD & HOLD_TIMEOUT => ST_LPRESSD
static state_t act_hold(fsm_t *fsm, const event_t *evt) {
  key_ctx_t *ctx = GET_CTX(fsm);
  // 发送 hold 事件
  event_t evt2 = {EVT_ID_KEY_HOLD, ctx->id};
  event_push(&evt2);

  return KEY_ST_LPRESSD;
}
// ST_PRESSED & RELEASE-DET => ST_IDLE
static state_t act_release(fsm_t *fsm, const event_t *evt) {
  key_ctx_t *ctx = GET_CTX(fsm);
  // 短按状态, 停止长按检测定时器
  timer_del(timer_id_lpress);
  timer_id_lpress = -1;

  // 短按, 发送短按和短松事件
  event_t evt1 = {EVT_ID_KEY_PRESSED, ctx->id};
  event_push(&evt1);
  event_t evt2 = {EVT_ID_KEY_RELEASED, ctx->id};
  event_push(&evt2);

  return KEY_ST_IDLE;
}
// ST_LPRESSED & RELEASE-DET => ST_IDLE
static state_t act_lrelease(fsm_t *fsm, const event_t *ev) {
  // 停止hold定时器
  timer_del(timer_id_hold);
  timer_id_hold = -1;
  return KEY_ST_IDLE;
}
