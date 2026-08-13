// 路由管理器实现（放在 common 目录）
#include "router.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define MAX_ROUTES 24
#define STACK_SIZE 8

// 运行时路由表（动态分配以减少 BSS 占用）
static struct {
  const char *name;
  route_init_fn_t init;
} *routes = NULL;

// 页面栈（运行时分配）
static lv_obj_t **stack = NULL;
static int stack_top = -1;

// 初始化路由子系统，动态分配注册表与栈
void router_init(void) {
  if (!routes) {
    routes = pvPortMalloc(sizeof(*routes) * MAX_ROUTES);
    if (routes)
      memset(routes, 0, sizeof(*routes) * MAX_ROUTES);
  }

  if (!stack) {
    stack = pvPortMalloc(sizeof(*stack) * STACK_SIZE);
    if (stack)
      memset(stack, 0, sizeof(*stack) * STACK_SIZE);
  }

  stack_top = -1;
}

// 注册一个路由
int router_register(const char *name, route_init_fn_t init_fn) {
  if (!routes)
    return -1; // 未初始化或内存不足
  for (int i = 0; i < MAX_ROUTES; i++) {
    if (routes[i].name == NULL) {
      routes[i].name = name;
      routes[i].init = init_fn;
      return 0;
    }
  }
  return -1; // 注册表已满
}

// 将静态路由表（位于 Flash）拷贝到运行时路由表中
void router_set_static_routes(const route_entry_t *static_routes, int count) {
  if (!routes)
    router_init();
  if (!routes)
    return; // 内存分配失败

  int n = count;
  if (n > MAX_ROUTES)
    n = MAX_ROUTES;
  for (int i = 0; i < n; i++) {
    routes[i].name = static_routes[i].name;
    routes[i].init = static_routes[i].init;
  }
}

// 根据名字查找路由初始化函数
static route_init_fn_t find_route(const char *name) {
  if (!routes)
    return NULL;
  for (int i = 0; i < MAX_ROUTES; i++) {
    if (routes[i].name && strcmp(routes[i].name, name) == 0)
      return routes[i].init;
  }
  return NULL;
}

// 替换当前页面为指定路由（不入栈）
int router_navigate(const char *name, void *params) {
  route_init_fn_t fn = find_route(name);
  if (!fn)
    return -1; // 未找到路由

  // 创建新屏幕并加载
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_scr_load(scr);

  // 调用视图初始化函数在新屏幕上创建控件
  fn(params);

  if (stack) {
    if (stack_top >= 0) {
      lv_obj_t *prev = stack[stack_top];
      if (prev && prev != scr) {
        lv_obj_del(prev);
        stack[stack_top] = scr;
      }
    } else {
      // 无基底时将当前屏幕作为基底入栈（若有空间）
      if (stack_top < STACK_SIZE - 1)
        stack[++stack_top] = scr;
    }
  }

  return 0;
}

// 将当前页面入栈并导航到新页面（用于可返回场景）
int router_push(const char *name, void *params) {
  route_init_fn_t fn = find_route(name);
  if (!fn)
    return -1; // 未找到路由
  if (!stack)
    return -2; // 未初始化或内存不足
  if (stack_top >= STACK_SIZE - 1)
    return -2; // 栈已满

  // 将当前活动屏幕保存到栈
  lv_obj_t *current = lv_scr_act();
  if (current) {
    if (stack_top < STACK_SIZE - 1)
      stack[++stack_top] = current;
  }

  // 创建并加载新屏幕
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_scr_load(scr);
  fn(params);
  return 0;
}

// 从栈中弹出并返回上一个页面
int router_pop(void) {
  if (!stack || stack_top < 0)
    return -1; // 栈为空或未初始化

  // 删除当前屏幕
  lv_obj_t *cur = lv_scr_act();
  if (cur)
    lv_obj_del(cur);

  // 加载栈顶的上一个屏幕
  lv_obj_t *prev = stack[stack_top--];
  if (prev)
    lv_scr_load(prev);
  return 0;
}
