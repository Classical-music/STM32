/*
 * LVGL 页面路由管理器。
 *
 * 路由器使用 LVGL screen 表示页面，并提供以下导航方式：
 * - router_navigate：替换当前页面，不修改历史栈；
 * - router_push：保存当前页面后进入新页面，支持返回；
 * - router_pop：销毁当前页面并恢复上一历史页面；
 * - router_reset：进入指定页面并清空全部历史，适合 Home 操作。
 *
 * 页面 screen 和路由名称保存在同一个历史项中，返回时可同时恢复页面和路由状态。
 */
#include "router.h"
#include <stdio.h>
#include <string.h>

/* 路由表和页面历史栈的最大容量。 */
#define MAX_ROUTES 24
#define STACK_SIZE 8

/* 路由表使用固定内存，避免为少量页面管理多组动态内存。 */
static route_entry_t routes[MAX_ROUTES];

/* screen 与路由名称放在同一个历史项中，避免维护两组并行数组。 */
static struct {
  lv_obj_t *screen;
  const char *route;
} history[STACK_SIZE];
static int stack_top = -1;

/*
 * 当前活动页面的路由名称。
 * 该指针指向路由注册时提供的长期有效字符串，不复制字符串内容。
 */
static const char *current_route = NULL;

typedef enum {
  ROUTER_OP_NONE,
  ROUTER_OP_NAVIGATE,
  ROUTER_OP_RESET,
  ROUTER_OP_PUSH,
  ROUTER_OP_POP,
} router_op_t;

/* 同一时刻只允许一个待执行的页面切换，避免事件重复触发时重复排队。 */
static struct {
  router_op_t op;
  const char *name;
  route_init_fn_t init;
  void *params;
} pending_request = {ROUTER_OP_NONE, NULL, NULL, NULL};

static void router_async_cb(void *user_data);

/*
 * 初始化路由子系统。
 *
 * 首次调用时动态申请路由表、页面栈和路由名称栈；已经申请的内存不会重复
 * 申请。每次调用都会将历史栈标记为空，并清除当前路由状态。
 */
void router_init(void) {
  /* 初始化导航状态：无历史页面，也没有已知的当前路由。 */
  memset(routes, 0, sizeof(routes));
  memset(history, 0, sizeof(history));
  stack_top = -1;
  current_route = NULL;
  pending_request.op = ROUTER_OP_NONE;
  pending_request.name = NULL;
  pending_request.init = NULL;
  pending_request.params = NULL;
}

/*
 * 向运行时路由表注册单个页面。
 *
 * 参数：
 * - name：页面唯一名称；
 * - init_fn：加载页面后负责创建页面控件的初始化函数。
 *
 * 返回 0 表示成功，返回 -1 表示路由器未初始化、内存申请失败或路由表已满。
 */
int router_register(const char *name, route_init_fn_t init_fn) {
  /* 从前向后寻找第一个空闲表项。 */
  for (int i = 0; i < MAX_ROUTES; i++) {
    if (routes[i].name == NULL) {
      routes[i].name = name;
      routes[i].init = init_fn;
      return 0;
    }
  }

  return -1;
}

/*
 * 批量加载静态路由表。
 *
 * static_routes 通常指向位于 Flash 的 const 数组。本函数只把各表项中的
 * 名称指针和初始化函数指针复制到运行时路由表，不复制字符串和函数本身。
 * 当传入数量超过 MAX_ROUTES 时，只加载路由表能够容纳的部分。
 */
void router_set_static_routes(const route_entry_t *static_routes, int count) {
  int n = count;
  if (n > MAX_ROUTES)
    n = MAX_ROUTES;

  for (int i = 0; i < n; i++) {
    routes[i].name = static_routes[i].name;
    routes[i].init = static_routes[i].init;
  }
}

/*
 * 根据路由名称查找对应的页面初始化函数。
 * 使用字符串内容比较，因此调用方不需要传入与注册时相同的字符串地址。
 */
static route_init_fn_t find_route(const char *name) {
  if (!name)
    return NULL;

  for (int i = 0; i < MAX_ROUTES; i++) {
    if (routes[i].name && strcmp(routes[i].name, name) == 0)
      return routes[i].init;
  }

  return NULL;
}

/*
 * 判断目标路由是否与当前活动页面一致。
 * current_route 尚未建立或 name 为空时，均视为不同路由。
 */
static int is_current_route(const char *name) {
  return current_route && name && strcmp(current_route, name) == 0;
}

static int queue_request(router_op_t op, const char *name,
                         route_init_fn_t init, void *params) {
  if (pending_request.op != ROUTER_OP_NONE)
    return 0;

  pending_request.op = op;
  pending_request.name = name;
  pending_request.init = init;
  pending_request.params = params;

  if (lv_async_call(router_async_cb, NULL) != LV_RES_OK) {
    pending_request.op = ROUTER_OP_NONE;
    pending_request.name = NULL;
    pending_request.init = NULL;
    pending_request.params = NULL;
    return -1;
  }

  return 0;
}

static void router_async_cb(void *user_data) {
  router_op_t op = pending_request.op;
  const char *name = pending_request.name;
  route_init_fn_t fn = pending_request.init;
  void *params = pending_request.params;

  (void)user_data;

  if (op == ROUTER_OP_NAVIGATE) {
    lv_obj_t *prev = lv_scr_act();
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);
    fn(params);
    current_route = name;
    if (prev && prev != scr)
      lv_obj_del(prev);
  } else if (op == ROUTER_OP_RESET) {
    lv_obj_t *current = lv_scr_act();
    lv_obj_t *scr = current;

    if (!is_current_route(name)) {
      scr = lv_obj_create(NULL);
      lv_scr_load(scr);
      fn(params);
      current_route = name;
      if (current && current != scr)
        lv_obj_del(current);
    }

    while (stack_top >= 0) {
      lv_obj_t *history_screen = history[stack_top].screen;
      history[stack_top].screen = NULL;
      history[stack_top].route = NULL;
      stack_top--;
      if (history_screen && history_screen != current && history_screen != scr)
        lv_obj_del(history_screen);
    }
  } else if (op == ROUTER_OP_PUSH) {
    lv_obj_t *current = lv_scr_act();
    if (current) {
      stack_top++;
      history[stack_top].screen = current;
      history[stack_top].route = current_route;
    }

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);
    fn(params);

    current_route = name;
  } else if (op == ROUTER_OP_POP) {
    lv_obj_t *cur = lv_scr_act();
    lv_obj_t *prev = history[stack_top].screen;
    current_route = history[stack_top].route;
    history[stack_top].screen = NULL;
    history[stack_top].route = NULL;
    stack_top--;

    if (prev)
      lv_scr_load(prev);
    if (cur && cur != prev)
      lv_obj_del(cur);
  }

  pending_request.op = ROUTER_OP_NONE;
  pending_request.name = NULL;
  pending_request.init = NULL;
  pending_request.params = NULL;
}

/*
 * 替换当前页面，但不改变历史栈。
 *
 * 若目标路由已经是当前路由，则直接返回成功，避免重复创建同一页面。
 * 否则先创建并加载新 screen，使页面初始化函数可以通过 lv_scr_act() 获得
 * 新 screen；初始化完成后再删除旧 screen。
 *
 * 返回值：0 成功，-1 路由不存在。
 */
int router_navigate(const char *name, void *params) {
  route_init_fn_t fn = find_route(name);
  if (!fn)
    return -1;
  if (is_current_route(name))
    return 0;

  return queue_request(ROUTER_OP_NAVIGATE, name, fn, params);
}

/*
 * 导航到指定页面并清空全部返回历史。
 *
 * 典型用途是底部 Home 按钮：无论用户经过多少页面，执行后都回到导航根
 * 页面，并且 Back 按钮不会再次进入旧业务页面。
 *
 * 若目标就是当前路由，不重新创建页面，但仍会清空历史栈。
 * 返回值：0 成功，-1 路由不存在。
 */
int router_reset(const char *name, void *params) {
  route_init_fn_t fn = find_route(name);
  if (!fn)
    return -1;

  return queue_request(ROUTER_OP_RESET, name, fn, params);
}

/*
 * 保存当前页面并进入目标页面。
 *
 * push 用于需要支持 Back 返回的普通页面跳转。当前 screen 及其路由名称会
 * 保存到相同栈下标，新页面退出时可由 router_pop() 原样恢复。
 * 若目标就是当前路由，则直接返回成功，不重复入栈或创建页面。
 *
 * 返回值：
 * -  0：跳转成功，或目标已经是当前页面；
 * - -1：目标路由不存在；
 * - -2：历史栈未初始化、内存申请失败或历史栈已满。
 */
int router_push(const char *name, void *params) {
  route_init_fn_t fn = find_route(name);
  if (!fn)
    return -1;
  if (is_current_route(name))
    return 0;
  if (stack_top >= STACK_SIZE - 1)
    return -2;

  return queue_request(ROUTER_OP_PUSH, name, fn, params) == 0 ? 0 : -2;
}

/*
 * 返回上一历史页面。
 *
 * 当前活动 screen 是离开的页面，需要先删除；栈顶 screen 已经包含原页面
 * 的全部控件，因此直接重新加载，不会再次调用该页面的初始化函数。
 * 同时从历史项恢复路由名称，保证后续重复路由判断正确。
 *
 * 返回值：0 成功，-1 历史栈未初始化或为空。
 */
int router_pop(void) {
  if (stack_top < 0)
    return -1;

  return queue_request(ROUTER_OP_POP, NULL, NULL, NULL);
}
