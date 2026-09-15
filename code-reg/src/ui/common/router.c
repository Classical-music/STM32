/*
 * LVGL 页面路由管理器。
 *
 * 全局只有一个默认屏幕（lv_scr_act()），由 views 取得并作为根节点；系统栏、
 * Dock 和当前页面都是它的子节点。路由器提供以下导航方式：
 * - router_navigate：替换当前页面，不修改历史栈；
 * - router_push：隐藏当前页面并保存，再创建新页面，支持返回；
 * - router_pop：把当前页面转入缓存并恢复上一历史页面；
 * - router_reset：进入指定页面并清空全部历史，适合 Home 操作。
 *
 * 页面容器和路由名称保存在同一个历史项中，返回时可同时恢复页面和路由状态。
 *
 * 页面缓存：
 * 硬件性能有限，创建页面是耗时操作，因此路由器在需要销毁页面的场景下并不
 * 立即销毁，而是把页面隐藏后放入缓存；再次进入同一路由时直接复用缓存实例，
 * 只取消隐藏并调用该路由的 refresh 回调刷新动态内容。缓存容量有上限，满时按
 * LRU 淘汰最久未使用的页面，淘汰前调用该路由的 destroy 回调，让页面模块清理
 * 自己的 static 控件引用。
 */
#include "router.h"
#include "../components/ui_page_base.h"
#include "system_bar.h"
#include <stdio.h>
#include <string.h>

/* 路由表、页面历史栈和页面缓存的最大容量。 */
#define MAX_ROUTES 24
#define STACK_SIZE 8
#define PAGE_CACHE_SIZE 2

/* 路由表使用固定内存，避免为少量页面管理多组动态内存。 */
static route_entry_t routes[MAX_ROUTES];

/* 承载系统栏、Dock 和页面的根 screen，由 router_init 通过 lv_scr_act() 取得。 */
static lv_obj_t *root_screen = NULL;

/* 页面容器与路由名称放在同一个历史项中，避免维护两组并行数组。 */
static struct {
  lv_obj_t *page;
  const char *route;
} history[STACK_SIZE];
static int stack_top = -1;

/* 当前显示中的页面容器。 */
static lv_obj_t *current_page = NULL;

/*
 * 当前活动页面的路由名称。
 * 该指针指向路由注册时提供的长期有效字符串，不复制字符串内容。
 */
static const char *current_route = NULL;

/*
 * 页面缓存。
 *
 * 缓存中的页面仍然挂在 root_screen 下，但一律带 LV_OBJ_FLAG_HIDDEN：隐藏
 * 对象不参与绘制，也会被 lv_indev_search_obj() 直接跳过，因此既不遮挡当前
 * 页面，也不会响应触摸。
 *
 * stamp 记录最近一次被使用（放入或取出）的先后顺序，缓存满时淘汰 stamp
 * 最小、也就是最久未被使用的页面。
 */
static struct {
  const char *route;
  lv_obj_t *page;
  uint32_t stamp;
} page_cache[PAGE_CACHE_SIZE];
static uint32_t cache_stamp;

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
  const route_entry_t *entry;
  void *params;
} pending_request = {ROUTER_OP_NONE, NULL, NULL};

static void router_async_cb(void *user_data);

/* 比较两个路由名称是否指向同一个页面，任一为空时视为不同。 */
static int route_name_equal(const char *a, const char *b) {
  if (a == NULL || b == NULL)
    return 0;

  return strcmp(a, b) == 0;
}

/*
 * 根据路由名称查找对应的注册表项。
 * 使用字符串内容比较，因此调用方不需要传入与注册时相同的字符串地址。
 * 未找到时返回 NULL。
 */
static const route_entry_t *find_entry(const char *name) {
  if (!name)
    return NULL;

  for (int i = 0; i < MAX_ROUTES; i++) {
    if (routes[i].name && strcmp(routes[i].name, name) == 0)
      return &routes[i];
  }

  return NULL;
}

/*
 * 真正销毁一个页面。
 *
 * 先让页面模块清理自己的 static 控件引用，再删除整棵对象树。只有缓存淘汰
 * 和路由子系统重置会走到这里，正常的页面切换一律改为转入缓存。
 */
static void page_destroy(const char *route, lv_obj_t *page) {
  const route_entry_t *entry;

  if (page == NULL)
    return;

  entry = find_entry(route);
  if (entry != NULL && entry->destroy != NULL)
    entry->destroy(page);

  lv_obj_del(page);
}

/* 清空一个缓存槽位。 */
static void cache_slot_reset(int index) {
  page_cache[index].route = NULL;
  page_cache[index].page = NULL;
  page_cache[index].stamp = 0;
}

/*
 * 从缓存中取出指定路由的页面，取出后立即脱离缓存，由调用方接管为当前页面。
 * 未命中时返回 NULL，调用方需要新建页面。
 */
static lv_obj_t *cache_take(const char *route) {
  for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
    if (page_cache[i].page == NULL)
      continue;
    if (!route_name_equal(page_cache[i].route, route))
      continue;

    lv_obj_t *page = page_cache[i].page;
    cache_slot_reset(i);
    return page;
  }

  return NULL;
}

/*
 * 把一个即将离开的页面放入缓存，而不是立即销毁。
 *
 * 页面被统一加上隐藏标志，保证它在缓存期间既不绘制也不响应触摸。
 * 同一路由最多保留一份缓存实例：已有旧实例时先销毁旧的，避免同一路由存在
 * 两份对象而其中一份再也取不出来。缓存已满时按 LRU 淘汰最久未使用的页面。
 */
static void cache_put(const char *route, lv_obj_t *page) {
  int slot = -1;

  if (page == NULL)
    return;

  lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);

  /* 同一路由的旧缓存实例直接销毁，腾出的槽位留给刚离开的页面。 */
  for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
    if (page_cache[i].page != NULL &&
        route_name_equal(page_cache[i].route, route)) {
      page_destroy(page_cache[i].route, page_cache[i].page);
      cache_slot_reset(i);
      break;
    }
  }

  /* 优先使用空槽。 */
  for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
    if (page_cache[i].page == NULL) {
      slot = i;
      break;
    }
  }

  /* 缓存已满：淘汰最久未使用的页面，也就是 stamp 最小的那一个。 */
  if (slot < 0) {
    for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
      if (slot < 0 || page_cache[i].stamp < page_cache[slot].stamp)
        slot = i;
    }

    page_destroy(page_cache[slot].route, page_cache[slot].page);
    cache_slot_reset(slot);
  }

  page_cache[slot].route = route;
  page_cache[slot].page = page;
  page_cache[slot].stamp = ++cache_stamp;
}

/* 销毁缓存中的全部页面，用于路由子系统重置。 */
static void cache_clear(void) {
  for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
    page_destroy(page_cache[i].route, page_cache[i].page);
    cache_slot_reset(i);
  }

  cache_stamp = 0;
}

/*
 * 取得一个可显示的页面容器：优先复用缓存实例，未命中才真正创建。
 *
 * 复用时页面控件已经存在，只取消隐藏并调用该路由的 refresh 回调更新动态
 * 内容，既不会重复创建控件，也省下了页面创建的耗时。
 * 新建时页面容器由 ui_page_base_create 统一定位到系统栏和 Dock 之间，并
 * 应用共用的基础样式；页面初始化函数只需要往里创建自己的控件。
 * 创建失败时返回 NULL。
 */
static lv_obj_t *page_create(const route_entry_t *entry, void *params) {
  lv_obj_t *page = cache_take(entry->name);

  if (page != NULL) {
    lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN);
    if (entry->refresh != NULL)
      entry->refresh(page, params);
    return page;
  }

  page = ui_page_base_create(root_screen);
  if (page == NULL)
    return NULL;

  entry->init(page, params);
  return page;
}

/* 用新页面替换当前页面，被替换的页面隐藏后转入缓存。 */
static void page_replace(const route_entry_t *entry, void *params) {
  lv_obj_t *prev = current_page;
  const char *prev_route = current_route;

  current_page = page_create(entry, params);
  current_route = entry->name;

  if (prev != NULL && prev != current_page)
    cache_put(prev_route, prev);
}

/*
 * 清空历史栈。
 *
 * 栈中的页面在 push 时已被隐藏，这里直接转交缓存管理而不真正销毁，因此
 * Home 之后再次进入这些页面时仍然可以复用。
 *
 * 按从最早到最近的顺序入缓存，保证最近访问过的页面拿到最大的 LRU 时间戳；
 * 否则缓存满时会反过来优先淘汰刚离开不久的页面。
 */
static void history_clear(void) {
  for (int i = 0; i <= stack_top; i++) {
    cache_put(history[i].route, history[i].page);
    history[i].page = NULL;
    history[i].route = NULL;
  }

  stack_top = -1;
}

/*
 * 判断目标路由是否与当前活动页面一致。
 * current_route 尚未建立或 name 为空时，均视为不同路由。
 */
static int is_current_route(const char *name) {
  return current_route && name && strcmp(current_route, name) == 0;
}

/*
 * 初始化路由子系统。
 *
 * 内部通过 lv_scr_act() 取得承载系统栏、Dock 和页面的默认屏幕，之后创建的
 * 所有页面都会作为它的子节点挂入。路由切换只在默认屏幕上换入换出页面子节点，
 * 不重新 load 屏幕。每次调用都会清空路由表、历史栈和页面缓存，并清除当前
 * 页面和当前路由状态。
 */
void router_init(void) {
  root_screen = lv_scr_act();

  /*
   * 路由表马上会被清空，缓存页面对应的 destroy 回调随之失效，
   * 因此必须在 memset 之前把缓存页面全部真正销毁。
   */
  cache_clear();

  memset(routes, 0, sizeof(routes));
  memset(history, 0, sizeof(history));
  stack_top = -1;
  current_page = NULL;
  current_route = NULL;
  pending_request.op = ROUTER_OP_NONE;
  pending_request.entry = NULL;
  pending_request.params = NULL;
}

/*
 * 向运行时路由表注册单个页面，并指定系统栏标题和页面缓存复用、销毁时的回调。
 *
 * 参数：
 * - name：页面唯一名称；
 * - title：显示在系统栏左侧的页面标题，可为 NULL；
 * - init_fn：页面容器首次创建后负责填充控件的初始化函数；
 * - refresh_fn：页面从缓存复用时刷新动态内容，可为 NULL 表示原样复用；
 * - destroy_fn：页面被缓存淘汰、真正销毁前清理模块状态，可为 NULL。
 *
 * 返回 0 表示成功，返回 -1 表示路由表已满。
 */
int router_register_ex(const char *name, const char *title,
                       route_init_fn_t init_fn,
                       route_refresh_fn_t refresh_fn,
                       route_destroy_fn_t destroy_fn) {
  /* 从前向后寻找第一个空闲表项。 */
  for (int i = 0; i < MAX_ROUTES; i++) {
    if (routes[i].name == NULL) {
      routes[i].name = name;
      routes[i].title = title;
      routes[i].init = init_fn;
      routes[i].refresh = refresh_fn;
      routes[i].destroy = destroy_fn;
      return 0;
    }
  }

  return -1;
}

/*
 * 批量加载静态路由表。
 *
 * static_routes 通常指向位于 Flash 的 const 数组。本函数只把各表项中的
 * 名称指针和回调函数指针复制到运行时路由表，不复制字符串和函数本身。
 * 当传入数量超过 MAX_ROUTES 时，只加载路由表能够容纳的部分。
 */
void router_set_static_routes(const route_entry_t *static_routes, int count) {
  int n = count;
  if (n > MAX_ROUTES)
    n = MAX_ROUTES;

  for (int i = 0; i < n; i++) {
    routes[i].name = static_routes[i].name;
    routes[i].title = static_routes[i].title;
    routes[i].init = static_routes[i].init;
    routes[i].refresh = static_routes[i].refresh;
    routes[i].destroy = static_routes[i].destroy;
  }
}

static int queue_request(router_op_t op, const route_entry_t *entry,
                         void *params) {
  if (pending_request.op != ROUTER_OP_NONE)
    return 0;

  pending_request.op = op;
  pending_request.entry = entry;
  pending_request.params = params;

  if (lv_async_call(router_async_cb, NULL) != LV_RES_OK) {
    pending_request.op = ROUTER_OP_NONE;
    pending_request.entry = NULL;
    pending_request.params = NULL;
    return -1;
  }

  return 0;
}

static void router_async_cb(void *user_data) {
  router_op_t op = pending_request.op;
  const route_entry_t *entry = pending_request.entry;
  void *params = pending_request.params;

  (void)user_data;

  if (op == ROUTER_OP_NAVIGATE) {
    page_replace(entry, params);
  } else if (op == ROUTER_OP_RESET) {
    /* 目标就是当前页面时不重建，只清空历史。 */
    if (!is_current_route(entry->name))
      page_replace(entry, params);

    history_clear();
  } else if (op == ROUTER_OP_PUSH) {
    /*
     * 当前页面留在对象树中以便返回时原样恢复，先隐藏避免与新页面重叠绘制。
     * 隐藏的对象不参与绘制和点击命中，只占用内存。
     */
    if (current_page != NULL && stack_top < STACK_SIZE - 1) {
      stack_top++;
      history[stack_top].page = current_page;
      history[stack_top].route = current_route;
      lv_obj_add_flag(current_page, LV_OBJ_FLAG_HIDDEN);
    }

    current_page = page_create(entry, params);
    current_route = entry->name;
  } else if (op == ROUTER_OP_POP) {
    lv_obj_t *leaving = current_page;
    const char *leaving_route = current_route;
    lv_obj_t *restored = history[stack_top].page;

    current_page = restored;
    current_route = history[stack_top].route;
    history[stack_top].page = NULL;
    history[stack_top].route = NULL;
    stack_top--;

    /* 恢复的页面已经包含原控件，只需取消隐藏，不再执行初始化函数。 */
    if (restored != NULL)
      lv_obj_clear_flag(restored, LV_OBJ_FLAG_HIDDEN);

    /* 离开的页面转入缓存，下次进入同一路由时可以直接复用。 */
    if (leaving != NULL && leaving != restored)
      cache_put(leaving_route, leaving);
  }

  /* 切换完成后，把当前路由注册的标题同步到系统栏。 */
  {
    const route_entry_t *active = find_entry(current_route);
    system_bar_set_title(active ? active->title : "");
  }

  pending_request.op = ROUTER_OP_NONE;
  pending_request.entry = NULL;
  pending_request.params = NULL;
}

/*
 * 替换当前页面，但不改变历史栈。
 *
 * 若目标路由已经是当前路由，则直接返回成功，避免重复创建同一页面。
 * 实际切换在 LVGL 空闲时异步执行：先取得并显示新页面（缓存命中时无需创建），
 * 再把旧页面隐藏转入缓存，避免切换过程中出现空白帧。
 *
 * 返回值：0 成功，-1 路由不存在。
 */
int router_navigate(const char *name, void *params) {
  const route_entry_t *entry = find_entry(name);
  if (!entry)
    return -1;
  if (is_current_route(name))
    return 0;

  return queue_request(ROUTER_OP_NAVIGATE, entry, params);
}

/*
 * 导航到指定页面并清空全部返回历史。
 *
 * 典型用途是底部 Home 按钮：无论用户经过多少页面，执行后都回到导航根
 * 页面，并且 Back 按钮不会再次进入旧业务页面。被清空的历史页面转入缓存，
 * 之后再次进入时仍可复用。
 *
 * 若目标就是当前路由，不重新创建页面，但仍会清空历史栈。
 * 返回值：0 成功，-1 路由不存在。
 */
int router_reset(const char *name, void *params) {
  const route_entry_t *entry = find_entry(name);
  if (!entry)
    return -1;

  return queue_request(ROUTER_OP_RESET, entry, params);
}

/*
 * 保存当前页面并进入目标页面。
 *
 * push 用于需要支持 Back 返回的普通页面跳转。当前页面容器及其路由名称会
 * 保存到相同栈下标，新页面退出时可由 router_pop() 原样恢复。
 * 若目标就是当前路由，则直接返回成功，不重复入栈或创建页面。
 *
 * 返回值：
 * -  0：跳转成功，或目标已经是当前页面；
 * - -1：目标路由不存在；
 * - -2：历史栈已满或请求排队失败。
 */
int router_push(const char *name, void *params) {
  const route_entry_t *entry = find_entry(name);
  if (!entry)
    return -1;
  if (is_current_route(name))
    return 0;
  if (stack_top >= STACK_SIZE - 1)
    return -2;

  return queue_request(ROUTER_OP_PUSH, entry, params) == 0 ? 0 : -2;
}

/*
 * 返回上一历史页面。
 *
 * 当前页面是离开的页面，隐藏后转入缓存；栈顶页面在 push 时只是被隐藏，控件
 * 全部保留，因此直接取消隐藏即可恢复，不会再次调用该页面的初始化函数。
 * 同时从历史项恢复路由名称，保证后续重复路由判断正确。
 *
 * 返回值：0 成功，-1 历史栈为空。
 */
int router_pop(void) {
  if (stack_top < 0)
    return -1;

  return queue_request(ROUTER_OP_POP, NULL, NULL);
}
