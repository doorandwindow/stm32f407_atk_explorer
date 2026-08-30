/**
 ******************************************************************************
 * @file    smart_periodic_task.c
 * @brief   small_smart 移植 - 控制状态机任务 (源 periodic 线程骨架)
 * @note    源工程 10ms 循环 + 1s 标志位消费的结构原样保留; 设备控制函数
 *          (源 Ctrl_Fan/Ctrl_Wet/Ctrl_JiaRe/...) 在探索者板上无继电器可操作,
 *          全部桩化为"决策写 g_smart_state, 不动硬件"。换 20 路继电器板时,
 *          桩函数体替换为 GPIO/PWM 输出即可, 决策逻辑与 UI 数据流不动。
 *          源工程的开机 45s 延迟、风机 2s 轮启防冲击等时序假设在此保留框架。
 ******************************************************************************
 */
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "main.h"
#include "iwdg.h"
#include "uart_dbg.h"
#include "smart_data.h"

osThreadId_t smartPeriodicHandle;
const osThreadAttr_t smartPeriodic_attributes = {
  .name = "smart_periodic",
  .stack_size = 512 * 4,                   /* 2KB: 沿源工程实证值 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* ---- 1s 级控制上下文 ---- */
static uint16_t ctl_10ms_cnt = 0;            /* 10ms 循环计数 -> 1s */
static uint32_t boot_delay_sec = 0;          /* 开机延迟计数 (源: 45s 后才启动控制) */
#define BOOT_DELAY_SEC  45U                  /* 源工程开机 45s 保护 */
static uint8_t  fan_step_idx = 0;            /* 风机轮启游标 (源: 每 2s 切一台防电流冲击) */

/* ---- 报警判据用比较 (真实现待 P5 与 smart_alarm 对齐) ---- */
static void ctrl_1s_tick(void);

/* ============================================================
 * 设备控制桩: 决策产出写 g_smart_state (0=停/1=运行), 不动硬件
 * 真机替换点: 函数体内按决策结果操作 DO/PWM, 并回读状态
 * ============================================================ */
static void ctrl_fan_stub(void)
{
  /* 决策示例: 平均温度 > 目标 +1.0°C 时轮启风机 (每 2s 一台), 降温到目标 -0.5°C 逐台停 */
  smart_state_t *s = (smart_state_t *)&g_smart_state;
  int16_t diff = g_smart_data.t_avr - g_tar_temp;

  if (diff > 10)
  {
    if (s->fan[fan_step_idx] == 0U)
    {
      s->fan[fan_step_idx] = 1U;   /* 真机: 此处写 DO, 2s 间隔由 Flag 节流 */
    }
    fan_step_idx = (uint8_t)((fan_step_idx + 1U) % 4U);   /* 前 4 台参与 */
  }
  else if (diff < -5)
  {
    for (uint8_t i = 0; i < 4U; i++) { s->fan[i] = 0U; }
    fan_step_idx = 0;
  }
}

static void ctrl_heating_stub(void)
{
  smart_state_t *s = (smart_state_t *)&g_smart_state;
  int16_t diff = g_smart_data.t_avr - g_tar_temp;

  /* 加热: 低于目标 1.0°C 开, 高于目标 0.3°C 停 (源温控模式简化) */
  s->heat[0] = (diff < -10) ? 1U : ((diff > 3) ? 0U : s->heat[0]);
}

static void ctrl_1s_tick(void)
{
  boot_delay_sec++;
  if (boot_delay_sec < BOOT_DELAY_SEC)
  {
    return;   /* 开机保护期: 不做控制决策 (源工程 45s) */
  }

  ctrl_fan_stub();
  ctrl_heating_stub();
}

/**
 * @brief  控制状态机任务: 10ms 骨架循环 + 消费 Flag_1s
 */
void StartSmartPeriodicTask(void *argument)
{
  (void)argument;
  dbg_printf("[smart] periodic started (10ms loop, hw-stubbed)\r\n");

  uint32_t next_wake = osKernelGetTickCount();
  for (;;)
  {
    next_wake += 10U;
    osDelayUntil(next_wake);
    HAL_IWDG_Refresh(&hiwdg);

    /* ---- 10ms 级: 真机在此跑小窗/死区计数器, 桩阶段空转 ---- */

    /* ---- 1s 级: 控制决策 ---- */
    if (Flag_1s != 0U)
    {
      Flag_1s = 0;
      ctrl_1s_tick();
    }
    /* Flag_5s/10s/30s/1m 由 smart_alarm/UI 各自消费, 此处不抢 */
  }
}
