/**
 ******************************************************************************
 * @file    dashboard_task.c
 * @brief   DeepSeek 用量仪表盘的数据轮询任务
 * @note    每 AIDASH_INTERVAL_MS 拉一次 PC 代理, 成功则写共享 g_dash 并置
 *          g_dash_ready=1 供 lvgl 侧刷新。socket 内有超时, 本任务全程喂狗。
 *          与 lvgl 任务的线程安全: 这里只写 g_dash(结构体拷贝), lvgl 任务只读。
 ******************************************************************************
 */

#include "ai_dash_api.h"

#include "main.h"
#include "iwdg.h"
#include "cmsis_os.h"
#include "uart_dbg.h"

#include <string.h>
#include <stdio.h>

/* ---- 共享数据(定义, 见 ai_dash_api.h extern) ---- */
ai_dash_data_t g_dash;
volatile uint8_t g_dash_ready = 0;
static volatile uint8_t s_poll_now = 0;

/** 请求立即刷新一次(由 UI 的 Refresh 按钮调用) */
void ai_dash_request_poll(void)
{
  s_poll_now = 1;
}

/**
 * @brief  数据轮询任务
 * @param  argument: Not used
 */
void StartDashboardTask(void *argument)
{
  (void)argument;

  dbg_printf("[dash] dashboardTask started, proxy %s:%u every %lums\r\n",
             AIDASH_PROXY_IP, AIDASH_PROXY_PORT, (unsigned long)AIDASH_INTERVAL_MS);

  uint32_t last = HAL_GetTick();
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */

    uint32_t now = HAL_GetTick();
    if (s_poll_now || (uint32_t)(now - last) >= AIDASH_INTERVAL_MS)
    {
      s_poll_now = 0;

      ai_dash_data_t tmp;
      memset(&tmp, 0, sizeof(tmp));
      if (ai_dash_poll(&tmp) == 0)
      {
        g_dash = tmp;          /* 结构体拷贝, lvgl 侧只读; 30s 一次, 极短暂撕裂可接受 */
        g_dash_ready = 1;
        last = now;
      }
      else
      {
        /* 失败: 保留上次数据, 不置 ready; 下个周期重试 */
      }
    }
    osDelay(20);
  }
}
