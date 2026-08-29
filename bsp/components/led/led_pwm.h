/**
 ******************************************************************************
 * @file    led_pwm.h
 * @brief   LED 调光模块 (TIM14_CH1 硬件 PWM)
 * @note    LED0 为 PF9, 是板上唯一具备硬件 PWM 的引脚 (TIM14_CH1, 复用 AF9)。
 *          亮度用 TIM14 通道 1 的真实 PWM 实现: 输出频率 1kHz, 0~100% 共 1000 级。
 *          LED 低电平点亮 (active-low), 通道极性配置为 TIM_OCPOLARITY_LOW。
 *          无中断, 任务直接修改比较寄存器 (HAL_TIM_SetCompare) 设亮度。
 ******************************************************************************
 */

#ifndef __LED_PWM_H
#define __LED_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 亮度等级总数 (0~LED_PWM_LEVELS 对应 0%~100%) */
#define LED_PWM_LEVELS   1000

/**
 * @brief  初始化 LED0(PF9) 硬件 PWM 调光
 * @note   配置 PF9 为 TIM14_CH1 (复用 AF9)、TIM14 为 PWM 输出 (1kHz), 初始占空比 0(灭)。
 *         从 FreeRTOS 任务里调用; 之后用 led_pwm_set_duty() 改亮度。
 * @retval 无
 */
void led_pwm_init(void);

/**
 * @brief  设置 LED 亮度
 * @param  percent: 0~100 (0=灭, 100=最亮)
 */
void led_pwm_set_duty(uint8_t percent);

/**
 * @brief  获取当前亮度 (0~100)
 */
uint8_t led_pwm_get_duty(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_PWM_H */
