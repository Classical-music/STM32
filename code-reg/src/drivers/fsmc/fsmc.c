#include "fsmc.h"

// FSMC寄存器地址 ========================================
// bank1有NE1~4,每个NE对应一个 BCR + TCR, 共8个寄存器
#define FSMC_BCR(neId) FSMC_Bank1->BTCR[(neId - 1) << 1]   // ne4
#define FSMC_BTR(neId) FSMC_Bank1->BTCR[(neId << 1) - 1]   // ne4
#define FSMC_BWTR(neId) FSMC_Bank1E->BWTR[(neId - 1) << 1] // ne4

// FSMC-BANK1-NE4 用于 LCD
void fsmc_ne4_init(void) {
  RCC->AHBENR |= RCC_AHBENR_FSMCEN; // FSMC

  // FSMC 时钟来自 HCLK, 72MHz
  // 寄存器清零
  FSMC_BCR(4) = 0x0;
  FSMC_BTR(4) = 0x0;
  FSMC_BWTR(4) = 0x0;

  // 时序配置 - 模式A
  // BCR片选控制 配置
  FSMC_BCR(4) |= FSMC_BCR4_EXTMOD; // 扩展模式使能, 允许读写时序不同
  FSMC_BCR(4) |= FSMC_BCR4_WREN;   // 写使能
  FSMC_BCR(4) |= fsmc_mwid_16;     // 数据总线宽度 01-16bit
  FSMC_BCR(4) &= ~FSMC_BCR4_MTYP;  // 存储器类型 00-SRAM (伪装)

  // BTR读片选时序 配置
  FSMC_BTR(4) &= ~FSMC_BTR4_ACCMOD; // 00-访问模式A
  FSMC_BTR(4) |= 15 << 8;           // 数据保持时间 （1/72M)*15=208ns
  FSMC_BTR(4) |= 1 & 0xF;           // 地址建立时间 (因F1有性能问题,设置1够了)

  // BWTR写片选时序 配置(EXTMOD=1)
  FSMC_BWTR(4) &= ~FSMC_BWTR4_ACCMOD; // 00-访问模式A
  FSMC_BWTR(4) |= 3 << 8;             // 数据保持时间
  FSMC_BWTR(4) |= 1 & 0xF;            // 地址建立时间 (因F1有性能问题,设置1够了)

  FSMC_BCR(4) |= FSMC_BCR4_MBKEN; // 使能Bank1-NE4
}

// FSMC-BANK1-NE3 用于 SRAM
void fsmc_ne3_init(void) {
  RCC->AHBENR |= RCC_AHBENR_FSMCEN;

  // 寄存器清零
  FSMC_BCR(3) = 0x0;
  FSMC_BTR(3) = 0x0;

  // 时序配置 - 模式1
  // BCR 片选控制 配置
  FSMC_BCR(3) &= ~FSMC_BCR3_EXTMOD; // 读写同时序
  FSMC_BCR(3) |= FSMC_BCR3_WREN;    // 写使能
  FSMC_BCR(3) |= fsmc_mwid_16;      // 数据总线宽度 01-16bit
  FSMC_BCR(3) &= ~FSMC_BCR3_MTYP;   // 存储器类型 00-SRAM (真实)

  // BTR 片选时序 配置
  FSMC_BTR(3) &= ~FSMC_BTR3_ACCMOD; // 00-访问模式A(无效)

  // 默认时序 (ADDSET/DATAST), 供 memtest 运行时对比
  fsmc_ne3_set_timing(FSMC_NE3_DEF_ADDSET, FSMC_NE3_DEF_DATAST);

  FSMC_BCR(3) |= FSMC_BCR3_MBKEN; // 使能Bank1-NE3
}

// 运行时修改 NE3 时序: 仅改 ADDSET[3:0] 与 DATAST[15:8], 保留其余字段.
// 用于 memtest 扫描时序找最优点. 收紧过快会导致外部 SRAM 读写错误.
void fsmc_ne3_set_timing(uint32_t addset, uint32_t datast) {
  uint32_t btr = FSMC_BTR(3);
  btr &= ~((0xFu << 0) | (0xFFu << 8)); // 清 ADDSET[3:0] DATAST[15:8]
  btr |= (addset & 0xFu) << 0;          // 地址建立时间
  btr |= (datast & 0xFFu) << 8;         // 数据建立时间
  FSMC_BTR(3) = btr;
}
