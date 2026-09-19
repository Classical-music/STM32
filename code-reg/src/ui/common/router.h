// 路由管理器头文件（放在 common 目录）
// 提供简单的页面注册与导航接口
//
// 全局只有一个默认屏幕（lv_scr_act()）；系统栏、Dock 和当前页面都是它的
// 子节点。路由切换时把旧页面从默认屏幕上换下，再把新页面挂上去，
// 因此页面本身是普通对象，可以自由设置位置和尺寸。
//
// 页面缓存：
// 硬件性能有限，创建页面是耗时操作，因此路由器不会在离开页面时立即销毁它，
// 而是把它隐藏后放入缓存；再次进入同一路由时直接复用缓存中的实例，只取消
// 隐藏并调用该路由的 refresh 回调刷新内容。缓存有容量上限，满时按 LRU 淘汰
// 最久未使用的页面，淘汰前调用该路由的 destroy 回调，让页面模块清理自己的
// static 控件引用。

#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "lvgl.h"

// 页面初始化函数类型
// page 为路由器已创建并定位好的页面容器，params 为可选参数指针
// 只在页面首次创建时调用一次，缓存复用不会再调用
typedef void (*route_init_fn_t)(lv_obj_t *page, void *params);

// 页面刷新函数类型
// 页面从缓存复用时调用，用 params 更新页面上的动态内容
// 控件本身已经存在，不要在这里重复创建控件
// 可以为 NULL，表示复用时原样显示进入缓存时的界面
typedef void (*route_refresh_fn_t)(lv_obj_t *page, void *params);

// 页面销毁函数类型
// 页面被缓存淘汰、真正执行 lv_obj_del 之前调用
// 页面模块在这里清空自己的 static 控件引用，避免下次重建时误判控件已存在
// 可以为 NULL
typedef void (*route_destroy_fn_t)(lv_obj_t *page);

// 页面显隐回调类型（hide / show 签名相同）
// hide：页面离开显示时调用（转入缓存，或 push 存入历史栈），用于暂停
//       lv_timer 等周期任务，避免隐藏页面白白刷新
// show：页面重新显示时调用（缓存命中复用，或 pop 从历史栈恢复），用于
//       恢复 hide 中暂停的任务
// 首次创建不调 show（定时器等都是新建的），从历史栈隐藏/恢复同样会触发，
// 因此回调要求可重入且成对。可以为 NULL
typedef void (*route_visibility_fn_t)(lv_obj_t *page);

// 初始化路由子系统，清空注册表、历史栈和页面缓存
// 内部通过 lv_scr_act() 取得承载系统栏、Dock 和页面的默认屏幕
void router_init(void);

// 注册一个路由，并指定系统栏标题与页面生命周期回调
// title 为显示在系统栏左侧的页面标题，允许传 NULL
// refresh_fn / hide_fn / show_fn / destroy_fn 均允许传 NULL
// 返回 0 成功，-1 失败（例如注册表已满）
int router_register_ex(const char *name, const char *title,
                       route_init_fn_t init_fn,
                       route_refresh_fn_t refresh_fn,
                       route_visibility_fn_t hide_fn,
                       route_visibility_fn_t show_fn,
                       route_destroy_fn_t destroy_fn);

// 替换当前页面为指定路由（不入栈），被替换的页面转入缓存
// 返回 0 成功，-1 未找到
int router_navigate(const char *name, void *params);

// 将当前页面入栈并导航到新页面（支持返回）
// 返回 0 成功，-1 未找到，-2 栈满
int router_push(const char *name, void *params);

// 从栈中弹出并返回上一个页面，离开的页面转入缓存
// 返回 0 成功，-1 栈为空
int router_pop(void);

// 清空全部页面历史并导航到指定页面
// 历史页面转入缓存而不是立即销毁
// 适用于 Home 操作，返回 0 成功，-1 未找到
int router_reset(const char *name, void *params);

// 静态路由表项类型（可放在只读区/Flash）
typedef struct {
  const char *name;
  const char *title; // 显示在系统栏左侧的页面标题，可为 NULL
  route_init_fn_t init;
  route_refresh_fn_t refresh;
  route_visibility_fn_t hide;  // 页面离开显示（转缓存/入历史栈）
  route_visibility_fn_t show;  // 页面重新显示（缓存复用/历史栈恢复）
  route_destroy_fn_t destroy;
} route_entry_t;

// 设置静态路由表（将路由表放在 Flash 中以节省 RAM）
// routes 指向只读 Flash 中的路由数组，count 为数组元素数量
void router_set_static_routes(const route_entry_t *routes, int count);

#endif
