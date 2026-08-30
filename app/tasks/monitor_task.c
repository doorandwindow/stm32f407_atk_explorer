/**
 ******************************************************************************
 * @file    monitor_task.c
 * @brief   系统状态监控任务 (1s 周期打印内存 + CPU 占用率)
 * @note    内存: heap_4 的 xPortGetFreeHeapSize / xPortGetMinimumEverFreeHeapSize。
 *          "峰值使用 = 总量 - 历史最小剩余" 由内核天然记录, 无需额外账本。
 *          CPU: 依赖 configGENERATE_RUN_TIME_STATS (TIM11 1MHz 统计时钟, 见
 *          bsp/components/rtstats/), 忙占比 = 100% - 空闲任务运行占比,
 *          1s 窗口整数 0.1% 精度 (无浮点)。
 *          单行输出走 dbg_printf 的 160B 栈缓冲, 注意字段总量勿超。
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "main.h"
#include "iwdg.h"
#include "uart_dbg.h"

/* ---- 打印周期 ---- */
#define MONITOR_PERIOD_MS   1000U

/**
 * @brief  系统状态监控任务
 * @param  argument: Not used
 */
void StartMonitorTask(void *argument)
{
  (void)argument;

  TaskStatus_t idle_st;
  uint32_t last_rt = 0U;        /* 上次采样的运行时钟总计数 (us) */
  uint32_t last_idle_rt = 0U;   /* 上次采样的空闲任务运行计数 (us) */

  dbg_printf("[mon] monitorTask started, period %ums\r\n", (unsigned)MONITOR_PERIOD_MS);

  uint32_t next_wake = osKernelGetTickCount();
  for (;;)
  {
    next_wake += MONITOR_PERIOD_MS;
    osDelayUntil(next_wake);

    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */

    /* ---- CPU 占用率: 本窗口内空闲任务运行时间的补数 ---- */
    uint32_t rt_now = portGET_RUN_TIME_COUNTER_VALUE();
    vTaskGetInfo(xTaskGetIdleTaskHandle(), &idle_st, pdFALSE, eRunning);
    uint32_t idle_rt = idle_st.ulRunTimeCounter;

    uint32_t dt      = rt_now - last_rt;        /* 窗口总运行时间 (us) */
    uint32_t d_idle  = idle_rt - last_idle_rt;  /* 窗口空闲时间 (us) */
    last_rt     = rt_now;
    last_idle_rt = idle_rt;

    /* 千分比整数运算, 打印时再拆成 x.y% (FPU 未开, 避开浮点) */
    uint32_t busy_pm = 0U;
    if (dt > 0U)
    {
      busy_pm = (uint32_t)(((uint64_t)(dt - d_idle) * 1000U) / dt);
    }

    /* ---- 堆状态: 当前 + 开机以来历史极值 ---- */
    uint32_t heap_total    = (uint32_t)configTOTAL_HEAP_SIZE;
    uint32_t heap_free     = (uint32_t)xPortGetFreeHeapSize();
    uint32_t heap_min_free = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    dbg_printf("[mon] cpu %u.%u%% | heap total %u used %u free %u peak_used %u min_free %u\r\n",
               (unsigned)(busy_pm / 10U), (unsigned)(busy_pm % 10U),
               (unsigned)heap_total,
               (unsigned)(heap_total - heap_free),
               (unsigned)heap_free,
               (unsigned)(heap_total - heap_min_free),
               (unsigned)heap_min_free);
  }
}
