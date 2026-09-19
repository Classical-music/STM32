#include "bsp_rtc.h"
#include <stdio.h>

/*
 * ============================================================================
 * STM32F103 RTC 配置原理
 * ============================================================================
 *
 * 一、时钟源
 * ----------------------------------------------------------------------------
 * F103 的 RTC 有 4 种可选时钟源(通过 RCC_BDCR 的 RTCSEL[1:0] 选择):
 *   - LSE (外部低速晶振, 通常 32.768 kHz): 精度高、功耗低, 是走时的首选
 *   - LSI (内部低速 RC, 约 40 kHz): 精度差, 作为无外部晶振时的退路
 *   - HSE/128 (外部高速晶振分频): 精度高但掉电即停, 不适合待机走时
 *   - No clock: 关闭
 * 本驱动选择 LSE = 32768 Hz, 因为它是唯一一个可以依靠 VBAT 纽扣电池在
 * 主电源掉电后继续振荡、从而保持走时的时钟源。
 * LSE 属于"备份域"(Backup Domain), 其寄存器(RCC_BDCR)和 RTC/BKP 一样,
 * 默认受写保护, 必须先使能 PWR 与 BKP 时钟并置位 PWR_CR.DBP 才能写入。
 *
 * 二、预分频值 (Prescaler, PRL)
 * ----------------------------------------------------------------------------
 * RTC 内部结构为:  时钟源 -> [16 位预分频器 PRL] -> [32 位向上计数器 CNT]
 * 计数器自增频率 = 时钟源频率 / (PRL + 1)
 * 为了让计数器"每秒 +1", 需要:
 *      PRL + 1 = 32768  =>  PRL = 32767 (0x7FFF)
 * F103 的 PRL 是 20 位宽, 拆成两个 16 位寄存器:
 *      RTC_PRLH 存放高 4 位 (本例为 0)
 *      RTC_PRLL  存放低 16 位 (本例为 0x7FFF)
 * 注意: PRL 与 CNT 都工作在 RTC 时钟域, 只有在"配置模式"下才能写入。
 *
 * 三、计数器清零与时间语义
 * ----------------------------------------------------------------------------
 * 配置完成后把 32 位计数器 CNT(CNTH:CNTL) 清 0, 表示时间基准的起点。
 * 之后计数器每秒自增 1, 溢出周期约 136 年, 软件无需处理翻转。
 * 本 bsp 层只暴露"原始秒计数", 不关心它是 1970 还是别的纪元;
 * 把计数值映射为真实时间戳(年月日时分秒)是上层 task 的职责。
 *
 * 四、配置流程 (bsp_rtc_init 的实际步骤)
 * ----------------------------------------------------------------------------
 * 1. 解锁备份域: 开 PWR/BKP 时钟 -> 置 PWR_CR.DBP 解除写保护。
 * 2. 判重: 读 BKP_DR1, 若等于魔数 RTC_BAK_MAGIC, 说明 RTC 之前已配置好
 *    且正在走时(复位不会清除备份域), 直接跳过后续配置以保留当前时间。
 * 3. 起振: 置 RCC_BDCR.LSEON, 轮询 RCC_BDCR.LSERDY 等待 LSE 稳定。
 *    加入超时, 防止晶振未焊接/损坏时无限死等。
 * 4. 选源: 配置 RTCSEL = LSE, 并置 RTCEN 使能 RTC 接口时钟。
 *    重要: RTCSEL 一旦写入即被锁定, 只有备份域复位(BDRST 或掉电)才能改写。
 * 5. 进配置模式: 先等 RTOFF=1(上一次操作完成), 再置 RTC_CRL.CNF=1。
 * 6. 写 PRL(32767) 并把 CNT 清 0。
 * 7. 退配置模式: 清 RTC_CRL.CNF=1(置 0), 再等 RTOFF=1。
 * 8. 等待同步: 清 RTC_CRL.RSF, 轮询直到 RSF 重新置 1, 表示预分频/计数器
 *    已从 RTC 时钟域同步到 APB 时钟域, 此后读到的 CNT 才有效。
 * 9. 写魔数到 BKP_DR1, 标记"已配置", 供下次复位时判重。
 *
 * 五、并发与撕裂保护
 * ----------------------------------------------------------------------------
 * 计数器是 32 位, 但硬件上是两个独立的 16 位寄存器(CNTH/CNTL), 读取瞬间
 * 可能恰好发生进位导致高低位不一致(撕裂)。
 *   - 读: 采用 "低-高-低" 三次读取, 两次低位一致才认为有效, 否则重试。
 *   - 写: 需进入配置模式, 用保存/恢复 PRIMASK 的临界区包住整个写序列,
 *          防止与读取或其它写操作交错。
 * ============================================================================
 */


// RTC 时钟源: 外部低速晶振 LSE = 32768 Hz
#define RTC_LSE_FREQ 32768
// 预分频值: 总分频 = PRL + 1, 32768 / 32768 = 1Hz (即 1 秒/计数)
#define RTC_PRL_VALUE (RTC_LSE_FREQ - 1)

// 备份域魔数: 存放在 BKP_DR1, 用于判断 RTC 是否已配置
// 备份域仅在 VDD 掉电且无 VBAT 时才会丢失, 复位不会清零
#define RTC_BAK_MAGIC 0xA5A5

// 各类等待的循环上限, 防止晶振未焊接时无限死等
#define RTC_LSE_TIMEOUT 0x8000U
#define RTC_RTOFF_TIMEOUT 0x8000U
#define RTC_RSF_TIMEOUT 0x8000U

static bool g_rtc_ready = false;

// 轮询 RTC_CRL 中的标志位直到置位(或超时)
// CRL: 配置寄存器, 用于配置 RTC 寄存器
// RTOFF: RTC 操作完成标志位
static bool bsp_rtc_wait_crl_flag(uint16_t flag, uint32_t timeout) {
  for (uint32_t i = 0; i < timeout; i++) {
    if (RTC->CRL & flag) {
      return true;
    }
  }
  return false;
}

// 等待上一次 RTC 操作完成(RTOFF)
static bool bsp_rtc_wait_rtoff(void) {
  return bsp_rtc_wait_crl_flag(RTC_CRL_RTOFF, RTC_RTOFF_TIMEOUT);
}

/* 进入/退出配置模式，通过设置/清除 RTC_CRL.CNF 来实现
 *   CRL: 配置寄存器, 用于配置 RTC 寄存器
 *   CNF: 配置标志位, 用于进入/退出配置模式
 * 每次修改 RTC 寄存器前, 要等待上一次操作完成 RTOFF=1
 * 修改完成后, 生效标识: 等待 RTOFF=1
 */
// 进入配置模式: 等待操作完成 -> 置 CNF -> 等待操作完成
static bool bsp_rtc_enter_config(void) {
  if (!bsp_rtc_wait_rtoff()) {
    return false;
  }
  RTC->CRL |= RTC_CRL_CNF;
  return bsp_rtc_wait_rtoff();
}

// 退出配置模式: 清 CNF -> 等待操作完成
static bool bsp_rtc_exit_config(void) {
  RTC->CRL &= (uint16_t)~RTC_CRL_CNF;
  return bsp_rtc_wait_rtoff();
}

// 等待RTC 计数器/预分频器从备份域时钟同步到 APB 时钟域
static bool bsp_rtc_wait_sync(void) {
  RTC->CRL &= (uint16_t)~RTC_CRL_RSF; // 清同步标志
  return bsp_rtc_wait_crl_flag(RTC_CRL_RSF, RTC_RSF_TIMEOUT);
}

// 首次配置备份域: 使能 PWR/BKP 时钟, 解除备份域写保护
static void bsp_rtc_backup_domain_unlock(void) {
  // PWR 意思是"电源管理器" & BKP 意思是"备份域管理器"
  // 1. 使能 PWR/BKP 时钟
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  // 2. 解除备份域写保护, DBP 意思是"备份域写保护"
  PWR->CR |= PWR_CR_DBP;
}

// 启动 LSE 并等待就绪
static bool bsp_rtc_lse_start(void) {
  // BDCR 意思是"备份域时钟寄存器", 用于配置 LSE = 32768 Hz
  // LSEON 意思是"使能 LSE", 用于启动 LSE 晶振
  // LERDY 意思是"LSE 就绪", 用于等待 LSE 晶振稳定

  // 首先使能 LSE, 然后轮询 LERDY 等待 LSE 晶振稳定
  RCC->BDCR |= RCC_BDCR_LSEON;
  for (uint32_t i = 0; i < RTC_LSE_TIMEOUT; i++) {
    if (RCC->BDCR & RCC_BDCR_LSERDY) {
      return true;
    }
  }
  return false; // LSE 晶振未起振(未焊接/损坏)
}

// 完整配置流程: 时钟源 + 预分频 + 计数器清零
static bool bsp_rtc_config(void) {
  /* 
   * 配置 RTC 寄存器: 时钟源 + 预分频 + 计数器清零
   * 配置顺序:
   * 1. 选择 LSE 作为 RTC 时钟源, 并使能 RTC
   * 2. 进入配置模式, 设置预分频并把计数器清零
   * 3. 等待寄存器同步
   * 4. 写入魔数, 标记备份域已配置
   */

  // 1. 选择 LSE 作为 RTC 时钟源, 并使能 RTC
  RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_LSE;
  RCC->BDCR |= RCC_BDCR_RTCEN;

  // 2. 进入配置模式, 设置预分频并把计数器清零
  if (!bsp_rtc_enter_config()) {
    return false;
  }
  // 配置 RTC 预分频器, 总分频 = PRL + 1
  RTC->PRLH = (uint16_t)((RTC_PRL_VALUE >> 16) & RTC_PRLH_PRL);
  RTC->PRLL = (uint16_t)(RTC_PRL_VALUE & RTC_PRLL_PRL);
  // 配置 RTC 计数器, 清零计数
  RTC->CNTH = 0;
  RTC->CNTL = 0;
  // 退出配置模式
  if (!bsp_rtc_exit_config()) {
    return false;
  }

  // 3. 等待寄存器同步
  if (!bsp_rtc_wait_sync()) {
    return false;
  }

  // 4. 写入魔数, 标记备份域已配置
  BKP->DR1 = RTC_BAK_MAGIC;
  return true;
}

void bsp_rtc_init(void) {
  // 1. 解除备份域写保护
  bsp_rtc_backup_domain_unlock();

  // 读取备份域 BKP_DR1, 检查是否为魔数, 即是否已配置过
  if (BKP->DR1 == RTC_BAK_MAGIC) {
    g_rtc_ready = true;
    return;
  }

  // RTC 时钟使用外部低速晶振 LSE = 32768 Hz, 因此先启动 LSE 并等待就绪
  if (!bsp_rtc_lse_start()) {
    printf("rtc: LSE ready timeout\r\n");
    g_rtc_ready = false;
    return;
  }

  // 3. 配置 RTC 寄存器
   g_rtc_ready = bsp_rtc_config();
  if (!g_rtc_ready) {
    printf("rtc: config failed\r\n");
  }
}

bool bsp_rtc_is_ready(void) { return g_rtc_ready; }

uint32_t bsp_rtc_get_counter(void) {
  uint16_t low1, low2, high;
  uint32_t counter;

  if (!g_rtc_ready) {
    return 0;
  }

  // CNTH/CNTL 为两个独立的 16 位寄存器, 读取瞬间可能发生进位
  // 采用 低-高-低 三次读取, 两次低 16 位一致才认为有效
  do {
    low1 = RTC->CNTL;
    high = RTC->CNTH;
    low2 = RTC->CNTL;
  } while (low1 != low2);

  counter = ((uint32_t)high << 16) | low1;
  return counter;
}

void bsp_rtc_set_counter(uint32_t counter) {
  uint32_t primask;

  if (!g_rtc_ready) {
    return;
  }

  // 写操作需要进配置模式, 期间屏蔽中断防止与读取交错
  primask = __get_PRIMASK();
  __disable_irq();

  if (bsp_rtc_enter_config()) {
    RTC->CNTH = (uint16_t)(counter >> 16);
    RTC->CNTL = (uint16_t)(counter & 0xFFFF);
    bsp_rtc_exit_config();
    bsp_rtc_wait_sync();
  }

  __set_PRIMASK(primask);
}
