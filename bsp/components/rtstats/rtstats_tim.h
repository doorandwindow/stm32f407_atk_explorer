/**
  ******************************************************************************
  * @file    rtstats_tim.h
  * @brief   FreeRTOS run-time stats 统计时钟 (TIM11 1MHz 自由计数)
  ******************************************************************************
  */
#ifndef __RTSTATS_TIM_H
#define __RTSTATS_TIM_H

#include <stdint.h>

void rtstats_rtclock_init(void);
uint32_t rtstats_rtclock_get(void);

#endif /* __RTSTATS_TIM_H */
