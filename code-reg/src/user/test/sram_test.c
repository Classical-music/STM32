#include "fsmc/fsmc.h"
#include "sram/bsp_sram.h"
#include "task.h"
#include "test.h"

// ---------------------------------------------------------------------------
// SRAM 读写速度基准测试 (内部 SRAM vs 外部 FSMC SRAM)
// 计时: Cortex-M3 DWT->CYCCNT 周期计数器 (72MHz 下 1 cycle ~= 13.9ns)
// 结果含义: ns/access 含少量循环开销, 用于对比内外存差异已足够
// ---------------------------------------------------------------------------
#define SPEED_TEST_SIZE 4096u // 每个区域测试 4KB

// 本工程 StdPeriph 的 stm32f10x.h/core_cm3.h 未提供 DWT 外设实例定义,
// 这里按 CMSIS 布局本地声明 (仅需 CTRL 与 CYCCNT 两个寄存器)
typedef struct {
  volatile uint32_t CTRL;   // 0x00
  volatile uint32_t CYCCNT; // 0x04
} dwt_reg_t;
#define DWT_REG ((dwt_reg_t *)0xE0001000UL)
#ifndef DWT_CTRL_CYCCNTENA_Msk
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)
#endif

// 内部 SRAM 测试区: 借用物理 RAM 末尾 4KB 安全余量 (见 main.c: 前 12KB
// 静态段 + 中 48KB heap_5, 0x2000F000 起最后 4KB 无任何使用者)。
// 不占 .bss, 避免撑爆 12KB 链接区域; 仅 memtest 运行期间使用。
#define IRAM_TEST_BASE 0x2000F000UL

static void dwt_init(void) {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 使能 trace
  DWT_REG->CYCCNT = 0;
  DWT_REG->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // 使能周期计数器
}

// cycles -> 每次访问 ns + 带宽 KB/s (bytes/us == KB/s)
static void show(const char *name, uint32_t cycles, uint32_t accesses,
                 uint32_t bytes) {
  uint32_t us = cycles / (SystemCoreClock / 1000000u);
  uint32_t ns = us * 1000u / accesses;
  uint32_t mbps = us ? bytes / us : 0; // bytes/us == MB/s
  printf("  %-14s %6lu ns/access  %8lu MB/s\r\n", name, (unsigned long)ns,
         (unsigned long)mbps);
}

// 同一地址重复写 8 次 x REPS 轮
#define SP_WR(name, TYPE)                                                      \
  do {                                                                         \
    volatile TYPE *q = (volatile TYPE *)base;                                  \
    const uint32_t reps = 2000u;                                               \
    __disable_irq();                                                           \
    uint32_t c0 = DWT_REG->CYCCNT;                                             \
    for (uint32_t r = 0; r < reps; r++) {                                      \
      *q = 0x5AU; *q = 0x5AU; *q = 0x5AU; *q = 0x5AU;                          \
      *q = 0x5AU; *q = 0x5AU; *q = 0x5AU; *q = 0x5AU;                          \
    }                                                                          \
    cycles = DWT_REG->CYCCNT - c0;                                             \
    __enable_irq();                                                            \
    show(name, cycles, reps * 8u, reps * 8u * sizeof(TYPE));                   \
  } while (0)

// 同一地址重复读 8 次 x REPS 轮
#define SP_RD(name, TYPE)                                                      \
  do {                                                                         \
    volatile TYPE *q = (volatile TYPE *)base;                                  \
    const uint32_t reps = 2000u;                                               \
    uint32_t acc = 0;                                                          \
    __disable_irq();                                                           \
    uint32_t c0 = DWT_REG->CYCCNT;                                             \
    for (uint32_t r = 0; r < reps; r++) {                                      \
      acc += *q; acc += *q; acc += *q; acc += *q;                              \
      acc += *q; acc += *q; acc += *q; acc += *q;                              \
    }                                                                          \
    cycles = DWT_REG->CYCCNT - c0;                                             \
    __enable_irq();                                                            \
    sink ^= acc;                                                               \
    show(name, cycles, reps * 8u, reps * 8u * sizeof(TYPE));                   \
  } while (0)

// 顺序遍历整块写 / 读
#define SEQ_WR(name, TYPE, REPS)                                               \
  do {                                                                         \
    volatile TYPE *p = (volatile TYPE *)base;                                  \
    const uint32_t n = SPEED_TEST_SIZE / sizeof(TYPE);                         \
    __disable_irq();                                                           \
    uint32_t c0 = DWT_REG->CYCCNT;                                             \
    for (uint32_t r = 0; r < (REPS); r++) {                                    \
      for (uint32_t i = 0; i < n; i++)                                         \
        p[i] = (TYPE)(i + r);                                                  \
    }                                                                          \
    cycles = DWT_REG->CYCCNT - c0;                                             \
    __enable_irq();                                                            \
    show(name, cycles, n * (REPS), n * (REPS) * sizeof(TYPE));                 \
  } while (0)

#define SEQ_RD(name, TYPE, REPS)                                               \
  do {                                                                         \
    volatile TYPE *p = (volatile TYPE *)base;                                  \
    const uint32_t n = SPEED_TEST_SIZE / sizeof(TYPE);                         \
    uint32_t acc = 0;                                                          \
    __disable_irq();                                                           \
    uint32_t c0 = DWT_REG->CYCCNT;                                             \
    for (uint32_t r = 0; r < (REPS); r++) {                                    \
      for (uint32_t i = 0; i < n; i++)                                         \
        acc += p[i];                                                           \
    }                                                                          \
    cycles = DWT_REG->CYCCNT - c0;                                             \
    __enable_irq();                                                            \
    sink ^= acc;                                                               \
    show(name, cycles, n * (REPS), n * (REPS) * sizeof(TYPE));                 \
  } while (0)

static void run_region(const char *title, void *base) {
  uint32_t cycles;
  uint32_t sink = 0;

  printf("\r\n== %s @ 0x%08lX ==\r\n", title, (unsigned long)(uintptr_t)base);
  SP_WR("write8 x1", uint8_t);
  SP_WR("write16 x1", uint16_t);
  SP_WR("write32 x1", uint32_t);
  SP_RD("read8 x1", uint8_t);
  SP_RD("read16 x1", uint16_t);
  SP_RD("read32 x1", uint32_t);
  SEQ_WR("seq write8", uint8_t, 4);
  SEQ_WR("seq write16", uint16_t, 4);
  SEQ_WR("seq write32", uint32_t, 4);
  SEQ_RD("seq read8", uint8_t, 4);
  SEQ_RD("seq read16", uint16_t, 4);
  SEQ_RD("seq read32", uint32_t, 4);
  (void)sink; // 读取结果已通过 volatile sink 汇总, 防止循环被优化
}

void sram_speed_test(void) {
  dwt_init();
  printf("\r\n===== SRAM speed benchmark (CPU %lu Hz) =====\r\n",
         (unsigned long)SystemCoreClock);
  run_region("Internal SRAM", (void *)IRAM_TEST_BASE);
  run_region("External SRAM (FSMC)", (void *)ESRAM_APP_BASE);
  printf("\r\n===== done =====\r\n");
}

// ---------------------------------------------------------------------------
// FSMC NE3 时序扫描: 逐档收紧 ADDSET/DATAST, 每档先做数据正确性校验,
// 通过才跑速度项, 全部结束后恢复默认时序 (保护挂在 NE3 上的 LVGL 池).
// ---------------------------------------------------------------------------

// 整块写图案再读回比对, 返回失配个数 (0 = 该档时序可靠)
static uint32_t verify_region(void *base, uint32_t size) {
  volatile uint16_t *p = (volatile uint16_t *)base;
  const uint32_t n = size / sizeof(uint16_t);
  uint32_t err = 0;
  for (uint32_t i = 0; i < n; i++)
    p[i] = (uint16_t)(0xA55A ^ i);
  for (uint32_t i = 0; i < n; i++) {
    if (p[i] != (uint16_t)(0xA55A ^ i))
      err++;
  }
  return err;
}

// 挂起(on=1)/恢复(on=0)持续读写外部 SRAM 内存池的任务:
// gui 任务跑 LVGL 渲染(池在外部 SRAM 前 512KB), touch 经 indev 访问 LVGL 对象。
// 扫描中过紧时序档位若让它们运行, 池内数据静默位翻转可能导致系统崩溃
// (此前曾在 DATAST=1 系列档位中途卡死, 疑为此因, 待本保护验证)。
// key->切页路径仅在按键时触发, 扫描期间不按键, 暂不挂起。
static void lvgl_pause(uint32_t on) {
  static TaskHandle_t handles[2]; // [0]=gui [1]=touch
  if (on) {
    handles[0] = xTaskGetHandle("gui");
    handles[1] = xTaskGetHandle("touch");
    for (uint32_t i = 0; i < 2; i++) {
      if (handles[i])
        vTaskSuspend(handles[i]);
    }
    printf("  (gui/touch tasks suspended during scan)\r\n");
  } else {
    for (uint32_t i = 0; i < 2; i++) {
      if (handles[i])
        vTaskResume(handles[i]);
      handles[i] = NULL;
    }
  }
}

void sram_timing_scan(void) {
  // 待扫档位: (ADDSET, DATAST), 单位 HCLK. {0,2} 即当前默认基线
  static const uint8_t cfgs[][2] = {
      {FSMC_NE3_DEF_ADDSET, FSMC_NE3_DEF_DATAST}, // 基线
      {0, 1},
  };
  dwt_init();
  printf("\r\n===== FSMC NE3 timing scan (CPU %lu Hz) =====\r\n",
         (unsigned long)SystemCoreClock);
  lvgl_pause(1);
  for (uint32_t k = 0; k < sizeof(cfgs) / sizeof(cfgs[0]); k++) {
    const uint32_t addset = cfgs[k][0];
    const uint32_t datast = cfgs[k][1];
    fsmc_ne3_set_timing(addset, datast);
    // 连续校验 3 轮, 提高低概率位翻转的检出率
    uint32_t err = 0;
    for (uint32_t v = 0; v < 3u && err == 0u; v++)
      err = verify_region((void *)ESRAM_APP_BASE, SPEED_TEST_SIZE);
    printf("\r\n-- ADDSET=%lu DATAST=%lu : %s --\r\n", (unsigned long)addset,
           (unsigned long)datast, err ? "UNSTABLE" : "verify OK");
    if (err) {
      printf("  verify errors: %lu, skipped\r\n", (unsigned long)err);
      continue;
    }
    run_region("External SRAM (FSMC)", (void *)ESRAM_APP_BASE);
  }
  // 无论成败, 先恢复默认时序, 再恢复 LVGL 任务 (避免其运行在过紧时序下)
  fsmc_ne3_set_timing(FSMC_NE3_DEF_ADDSET, FSMC_NE3_DEF_DATAST);
  lvgl_pause(0);
  printf("\r\n===== scan done, restored ADDSET=%lu DATAST=%lu =====\r\n",
         (unsigned long)FSMC_NE3_DEF_ADDSET,
         (unsigned long)FSMC_NE3_DEF_DATAST);
}

// 测试 sram 读写
void sram_test(void) {
  // 测试8bit读写
  for (uint8_t i = 0; i < 10; i++) {
    SRAM_WRITE8(i, i + '0');
  }
  for (uint8_t i = 0; i < 10; i++) {
    printf("sram8[%d] = %c\n", i, SRAM_READ8(i));
  }
  // 测试16bit读写
  for (uint8_t i = 0; i < 10; i += 2) {
    SRAM_WRITE16(i, (i + 1) << 8 | i);
  }
  for (uint8_t i = 0; i < 10; i += 1) {
    printf("sram16[%d] = %x\n", i, SRAM_READ16(i));
  }
}
