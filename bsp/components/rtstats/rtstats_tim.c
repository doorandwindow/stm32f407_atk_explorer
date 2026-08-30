/**
 ******************************************************************************
 * @file    rtstats_tim.c
 * @brief   FreeRTOS run-time stats 统计时钟 (TIM11 1MHz 自由计数)
 * @note    为 configGENERATE_RUN_TIME_STATS 提供高分辨率运行时钟, 时序设计:
 *
 *    - TIM11 挂 APB2, 定时器时钟 168MHz
 *    - 预分频 167 => 计数时钟 1MHz (tick 1kHz 的 1000 倍, 满足内核 >=10x 要求)
 *    - 16 位计数器每 65.536ms 溢出一次, 溢出中断累加高位 => 32 位虚拟计数器
 *      (1MHz 下 uint32 约 71.6 分钟回绕, 内核差值为无符号减法, 单次回绕无碍)
 *
 *  接线: FreeRTOSConfig.h 中 (config/ 与 Inc/ 两份需同步修改)
 *    portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() -> rtstats_rtclock_init()
 *    portGET_RUN_TIME_COUNTER_VALUE()         -> rtstats_rtclock_get()
 *  内核在 vTaskStartScheduler() 里调用 init, 此时 MX_TIM11_Init 已跑过
 *  (时钟已使能), 这里只重配分频并启动。
 *
 *  中断不经过 HAL_TIM_PeriodElapsedCallback —— 它已被 main.c 的 TIM7 时基
 *  占用, 在 IRQHandler 里直接判/清 UIF, 只累加计数、不调任何 FreeRTOS API,
 *  中断优先级可任意 (取最低 15)。
 ******************************************************************************
 */

#include "rtstats_tim.h"

#include "tim.h"
#include "stm32f4xx_hal.h"

/* ---- 时序参数 ---- */
#define RTSTATS_APB2_CLK_HZ   168000000UL  /* APB2 定时器时钟 168MHz */
#define RTSTATS_CNT_CLK_HZ    1000000UL    /* 预分频后计数时钟 1MHz */
#define RTSTATS_PSC           ((uint16_t)((RTSTATS_APB2_CLK_HZ / RTSTATS_CNT_CLK_HZ) - 1U)) /* 167 */

static volatile uint32_t s_overflow = 0;   /* 16 位溢出次数 (虚拟计数器高 16 位) */

/**
 * @brief  初始化 TIM11 为 1MHz 自由计数器 (供 run-time stats)
 */
void rtstats_rtclock_init(void)
{
  /* 重配分频: PSC 是影子寄存器, 写入后需产生一次 UEV 才生效 */
  __HAL_TIM_SET_PRESCALER(&htim11, RTSTATS_PSC);
  __HAL_TIM_SET_AUTORELOAD(&htim11, 0xFFFFu);
  __HAL_TIM_SET_COUNTER(&htim11, 0U);
  htim11.Instance->EGR = TIM_EGR_UG;                    /* 立即重载 PSC (会置 UIF) */
  __HAL_TIM_CLEAR_FLAG(&htim11, TIM_FLAG_UPDATE);       /* 清掉上面 UG 产生的 UIF, 防假中断 */

  s_overflow = 0U;

  HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, 15U, 0U);
  HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
  HAL_TIM_Base_Start_IT(&htim11);   /* 使能 UIE + 启动计数 */
}

/**
 * @brief  读 32 位虚拟运行计数值 (1 tick = 1us)
 * @note   先读溢出数、再读 CNT、复读溢出数, 不一致说明期间发生溢出, 重读
 */
uint32_t rtstats_rtclock_get(void)
{
  uint32_t high1, high2, cnt;

  do
  {
    high1 = s_overflow;
    cnt   = __HAL_TIM_GET_COUNTER(&htim11);
    high2 = s_overflow;
  } while (high1 != high2);

  return (high1 << 16) | cnt;
}

/**
 * @brief  TIM11 溢出中断: 累加虚拟计数器高位
 */
void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
  if (__HAL_TIM_GET_FLAG(&htim11, TIM_FLAG_UPDATE) != RESET)
  {
    __HAL_TIM_CLEAR_FLAG(&htim11, TIM_FLAG_UPDATE);
    s_overflow++;
  }
}
