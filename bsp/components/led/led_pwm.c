/**
 ******************************************************************************
 * @file    led_pwm.c
 * @brief   LED 调光模块 (TIM14_CH1 硬件 PWM)
 * @note    实现见 led_pwm.h。
 *
 *  时序设计:
 *    - APB1 定时器时钟 = 42MHz x 2 = 84MHz (APB1 分频 4 时定时器时钟翻倍)
 *    - 预分频 83 => 计数时钟 1MHz
 *    - 自动重载 999 => PWM 输出频率 = 1MHz / 1000 = 1kHz
 *    - 比较值 CCR 取值 0~1000 => 1000 级亮度
 *
 *  极性:
 *    - LED 低电平点亮 (active-low), 通道极性配 TIM_OCPOLARITY_LOW:
 *        CNT < CCR 输出低(亮), CNT > CCR 输出高(灭)。
 *      => CCR 越大越亮; CCR=0 全灭(恒高), CCR=1000 全亮(恒低)。
 *
 *  演进说明: 早期版本用 TIM13 中断做"软 PWM", 因为当时亮度落在 PF10(无硬件 PWM)。
 *  对换任务后亮度落在 PF9=TIM14_CH1, 改用真实硬件 PWM, 不再需要任何 PWM 中断。
 ******************************************************************************
 */

#include "led_pwm.h"

#include "board.h"
#include "tim.h"
#include "stm32f4xx_hal.h"

/* ---- 时序参数 ---- */
#define LED_PWM_APB1_CLK_HZ   84000000UL   /* APB1 定时器时钟 84MHz */
#define LED_PWM_CNT_CLK_HZ    1000000UL    /* 预分频后计数时钟 1MHz */
#define LED_PWM_FREQ_HZ       1000UL       /* PWM 刷新率 1kHz */
#define LED_PWM_ARR           ((uint16_t)((LED_PWM_CNT_CLK_HZ / LED_PWM_FREQ_HZ) - 1U))  /* 999 */
#define LED_PWM_CCR_MAX       ((uint32_t)LED_PWM_ARR + 1U)    /* CCR 上限 1000 (全亮) */

/**
 * @brief  初始化 LED0(PF9) 硬件 PWM 调光
 */
void led_pwm_init(void)
{
  /* ---- PF9 -> TIM14_CH1 (复用 AF9), 推挽, 低电平点亮 ---- */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin       = BOARD_LED0_PIN;
  gpio.Mode      = GPIO_MODE_AF_PP;
  gpio.Pull      = GPIO_NOPULL;
  gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF9_TIM14;
  HAL_GPIO_Init(BOARD_LED0_GPIO, &gpio);

  /* ---- 重配 TIM14 为 PWM (复用于 MX_TIM14_Init 已启用的时钟) ---- */
  htim14.Init.Prescaler        = (uint16_t)((LED_PWM_APB1_CLK_HZ / LED_PWM_CNT_CLK_HZ) - 1U); /* 83 */
  htim14.Init.CounterMode      = TIM_COUNTERMODE_UP;
  htim14.Init.Period           = LED_PWM_ARR;                       /* 999 */
  htim14.Init.ClockDivision    = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim14) != HAL_OK)
  {
    return;
  }

  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode     = TIM_OCMODE_PWM1;
  sConfigOC.Pulse      = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;   /* 低电平点亮: CNT<CCR 输出低(亮) */
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    return;
  }

  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);   /* 初始 0% -> 全灭 */
  if (HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1) != HAL_OK)
  {
    return;
  }
}

/**
 * @brief  设置亮度
 * @param  percent: 0~100
 */
void led_pwm_set_duty(uint8_t percent)
{
  uint32_t ccr;
  if (percent > 100U)
  {
    percent = 100U;
  }
  ccr = ((uint32_t)percent * LED_PWM_CCR_MAX) / 100U;
  if (ccr > LED_PWM_CCR_MAX)
  {
    ccr = LED_PWM_CCR_MAX;
  }
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, ccr);
}

/**
 * @brief  获取当前亮度 (0~100)
 */
uint8_t led_pwm_get_duty(void)
{
  uint32_t ccr = __HAL_TIM_GET_COMPARE(&htim14, TIM_CHANNEL_1);
  if (ccr > LED_PWM_CCR_MAX)
  {
    ccr = LED_PWM_CCR_MAX;
  }
  return (uint8_t)(ccr * 100U / LED_PWM_CCR_MAX);
}
