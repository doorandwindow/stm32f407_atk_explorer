/**
 ******************************************************************************
 * @file    smart_alarm_task.c
 * @brief   small_smart 移植 - 报警巡检任务 (源 alarm 线程骨架)
 * @note    消费 Flag_5s (源工程报警巡检 1s/5s 节拍, 这里取 5s 档降噪)。
 *          报警编号沿用源工程 __AlarmCnt=11 约定, P3 先实现高温/低温/传感器
 *          故障三类; 电话/短信/TTS 联动(源 APP_Call + AIR724)整链不移植。
 *          UI 侧 (P5 报警页) 只读 g_alarm_list / g_alarm_all。
 ******************************************************************************
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "main.h"
#include "iwdg.h"
#include "uart_dbg.h"
#include "smart_data.h"

/* 报警编号: 共享定义见 smart_data.h (UI 报警页按同表映射文案) */

osThreadId_t smartAlarmHandle;
const osThreadAttr_t smartAlarm_attributes = {
  .name = "smart_alarm",
  .stack_size = 512 * 4,                   /* 2KB: 沿源工程实证值 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* 单项报警置位/解除: 沿源工程 FirstFlag 思路简化为沿触发 */
static void alarm_set(uint8_t id, uint8_t active)
{
  smart_alarm_t *a = (smart_alarm_t *)&g_alarm_list[id];
  if (active != 0U)
  {
    if (a->active == 0U)
    {
      a->active = 1U;
      a->sec   = Timer_SecNow;
      dbg_printf("[alm] #%u ON @%us\r\n", (unsigned)id, (unsigned)a->sec);
    }
  }
  else if (a->active != 0U)
  {
    a->active = 0U;
    dbg_printf("[alm] #%u OFF @%us\r\n", (unsigned)id, (unsigned)Timer_SecNow);
  }
}

/* 巡检一次: 高/低温 + 传感器故障 */
static void alarm_patrol(void)
{
  int16_t diff = g_smart_data.t_avr - g_tar_temp;

  alarm_set(SMART_ALM_HIGH_TEMP, (diff > 50) ? 1U : 0U);   /* > 目标 +5.0°C */
  alarm_set(SMART_ALM_LOW_TEMP,  (diff < -50) ? 1U : 0U);  /* < 目标 -5.0°C */

  {
    const smart_error_t *e = (const smart_error_t *)&g_smart_err;
    uint8_t fault = (e->temp[0] || e->temp[1] || e->temp[2] ||
                     e->humi || e->co2 || e->pa) ? 1U : 0U;
    alarm_set(SMART_ALM_SENS_FAULT, fault);
  }

  /* 汇总标志 */
  {
    uint8_t any = 0;
    for (uint8_t i = 0; i < SMART_ALARM_CNT; i++)
    {
      if (g_alarm_list[i].active != 0U) { any = 1U; break; }
    }
    g_alarm_all = any;
  }
}

void StartSmartAlarmTask(void *argument)
{
  (void)argument;
  dbg_printf("[smart] alarm started (5s patrol)\r\n");

  uint32_t next_wake = osKernelGetTickCount();
  for (;;)
  {
    next_wake += 100U;
    osDelayUntil(next_wake);
    HAL_IWDG_Refresh(&hiwdg);

    if (Flag_5s != 0U)
    {
      Flag_5s = 0;
      alarm_patrol();
    }
  }
}
