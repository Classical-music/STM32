#ifndef __TEST_BEEP_H
#define __TEST_BEEP_H

/**
 * @brief 执行一轮蜂鸣器频率自测后返回：音高阶梯 -> 频率上下限探测 -> 长响收尾
 * @note  内部按 1ms 节拍定时（bsp/tick），一轮约 7s；调用方（main）自己决定跑几轮或是否循环。
 * @note  每档音高都走 BEEP_SetTone()，只听得出"嘟嘟"节奏而听不出音调高低，
 *        说明 PWM 频率没生效，检查 TIM3 时钟与 PB0 的复用配置。
 * @note  探测段的前几档（1Hz/100Hz/500Hz/1000Hz）低于 16 位定时器的下限，
 *        会被 BEEP_SetTone() 拒绝，预期就是不出声，只看到 LED 继续跳。
 */
void Beep_Test(void);

#endif /* __TEST_BEEP_H */
