/**
 * @file  test_seg.h
 * @brief 数码管（PA0~PA7，共阳极）上电自测：跑一轮完整序列
 *
 * 与 bsp 层分开存放，正式功能定型后可以整目录从工程里剔除，不影响驱动。
 */

#ifndef __TEST_SEG_H
#define __TEST_SEG_H

/**
 * @brief 执行一轮自测后返回：逐段扫描 -> 字符表 -> 小数点 -> 全亮/全灭
 * @note  内部按 1ms 节拍定时（bsp/tick），一轮约 10s；调用方（main）自己决定跑几轮或是否循环。
 * @note  现象对照见 test_seg.c 文件头注释。
 */
void Seg_Test(void);

#endif /* __TEST_SEG_H */
