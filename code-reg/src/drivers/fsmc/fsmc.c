#include "fsmc.h"

// LCD_FSMC寄存器地址 ========================================
// bank1有NE1~4,每个NE对应一个 BCR + TCR, 共8个寄存器
#define FSMC_BCR(ctx) ctx->bank->BTCR[(ctx->neId - 1) << 1]   // ne4
#define FSMC_BTR(ctx) ctx->bank->BTCR[(ctx->neId << 1) - 1]   // ne4
#define FSMC_BWTR(ctx) ctx->bankE->BWTR[(ctx->neId - 1) << 1] // ne4

void fsmc_init(fsmc_ctx_t *ctx) {
  RCC->AHBENR |= RCC_AHBENR_FSMCEN; // FSMC

  // FSMC 时钟来自 HCLK, 72MHz
  // 寄存器清零
  FSMC_BCR(ctx) = 0x0;
  FSMC_BTR(ctx) = 0x0;
  FSMC_BWTR(ctx) = 0x0;

  // 时序配置 - 模式A
  // BCR片选控制 配置
  FSMC_BCR(ctx) |= FSMC_BCR4_EXTMOD; // 扩展模式使能, 允许读写时序不同
  FSMC_BCR(ctx) |= FSMC_BCR4_WREN;   // 写使能
  FSMC_BCR(ctx) |= ctx->mwid;        // 数据总线宽度 01-16bit
  FSMC_BCR(ctx) &= ~FSMC_BCR4_MTYP;  // 存储器类型 00-SRAM (伪装)

  // BTR读片选时序 配置
  FSMC_BTR(ctx) &= ~FSMC_BTR4_ACCMOD; // 00-访问模式A
  FSMC_BTR(ctx) |= 15 << 8;           // 数据保持时间 （1/72M)*15=208ns
  FSMC_BTR(ctx) |= 1 & 0xF;           // 地址建立时间 (因F1有性能问题,设置1够了)

  // BWTR写片选时序 配置(EXTMOD=1)
  FSMC_BWTR(ctx) &= ~FSMC_BWTR4_ACCMOD; // 00-访问模式A
  FSMC_BWTR(ctx) |= 3 << 8;             // 数据保持时间
  FSMC_BWTR(ctx) |= 1 & 0xF; // 地址建立时间 (因F1有性能问题,设置1够了)

  FSMC_BCR(ctx) |= FSMC_BCR4_MBKEN; // 使能Bank1
}
