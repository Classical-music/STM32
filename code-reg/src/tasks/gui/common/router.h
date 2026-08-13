// 路由管理器头文件（放在 common 目录）
// 提供简单的页面注册与导航接口，基于 LVGL 屏幕（screen）实现

#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "lvgl.h"

// 页面初始化函数类型，接受可选参数指针
typedef void (*route_init_fn_t)(void *params);

// 初始化路由子系统，清空注册表与栈
void router_init(void);

// 注册一个路由，返回 0 成功，-1 失败（例如注册表已满）
int router_register(const char *name, route_init_fn_t init_fn);

// 替换当前页面为指定路由（不入栈）
// 返回 0 成功，-1 未找到
int router_navigate(const char *name, void *params);

// 将当前页面入栈并导航到新页面（支持返回）
// 返回 0 成功，-1 未找到，-2 栈满
int router_push(const char *name, void *params);

// 从栈中弹出并返回上一个页面
// 返回 0 成功，-1 栈为空
int router_pop(void);

// 静态路由表项类型（可放在只读区/Flash）
typedef struct {
  const char *name;
  route_init_fn_t init;
} route_entry_t;

// 设置静态路由表（将路由表放在 Flash 中以节省 RAM）
// routes 指向只读 Flash 中的路由数组，count 为数组元素数量
void router_set_static_routes(const route_entry_t *routes, int count);

#endif
