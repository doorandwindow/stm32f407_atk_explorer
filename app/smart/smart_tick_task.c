/**
 ******************************************************************************
 * @file    smart_tick_task.c
 * @brief   small_smart 移植 - 节拍任务 (替代源工程 3 个 rt_timer)
 * @note    1s 循环置 Flag_1s/5s/10s/30s/1m/1h/Flag_Fresh (消费者清 0),
 *          并驱动桩数据源; 100ms/200ms 级节拍(小窗/继电器死区)属控制硬件,
 *          待接真机时并入 smart_periodic 的 10ms 循环计数, 此处不管。
 *          BelowNormal: 只做置位, 不与 UI/控制抢 CPU。
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
#include "smart_sim.h"

osThreadId_t smartTickHandle;
const osThreadAttr_t smartTick_attributes = {
  .name = "smart_tick",
  .stack_size = 256 * 4,                   /* 1KB: 置位 + sim 算术, 无调用链 */
  .priority = (osPriority_t) osPriorityBelowNormal,
};

void StartSmartTickTask(void *argument)
{
  (void)argument;
  dbg_printf("[smart] tick started (1s beat)\r\n");

  uint32_t next_wake = osKernelGetTickCount();
  for (;;)
  {
    next_wake += 1000U;
    osDelayUntil(next_wake);
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */

    /* ---- 软时钟与节拍标志 (源 APP_Timer.c app_timer_entry 精简) ---- */
    Timer_SecNow++;
    if (Timer_SecNow % 60U == 0U)
    {
      Timer_MinNow++;
      Flag_1m = 1;
      if (Timer_SecNow % 3600U == 0U) { Flag_1h = 1; }
    }
    Flag_1s = 1;
    if (Timer_SecNow % 5U  == 0U) { Flag_5s = 1; }
    if (Timer_SecNow % 10U == 0U) { Flag_10s = 1; Flag_Fresh = 1; }
    if (Timer_SecNow % 30U == 0U) { Flag_30s = 1; }

    /* ---- 桩数据源 (真机换传感器采集) ---- */
    smart_sim_update();
  }
}
