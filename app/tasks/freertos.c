/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "gt9147.h"
#include "lvgl.h"
#include "uart_dbg.h"
#include "iwdg.h"
#include "demo_main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,   /* 2KB: 任务里跑 MX_LWIP_Init(), 调用链 ~300B; 512B 会栈溢出踩坏 TCB →
                              调度器恢复坏上下文 → INVPC HardFault → IWDG 复位循环 (2026-08-27 二分已证实) */
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for lvglTestTask */
osThreadId_t lvglTestTaskHandle;
const osThreadAttr_t lvglTestTask_attributes = {
  .name = "lvglTestTask",
  .stack_size = 4096 * 4,   /* 16KB: 由 4KB 提升, 修复栈溢出导致的 HardFault(INVPC) */
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartLvglTestTask(void *argument);
extern void lv_port_disp_init(void);
extern void lv_port_disp_get_stats(uint32_t *count, uint32_t *pixels);
extern void lv_port_indev_init(void);
extern void demo_advanced_update(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  lvglTestTaskHandle = osThreadNew(StartLvglTestTask, NULL, &lvglTestTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s (LSI 32kHz/16, Reload 4095) */
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
  * @brief  LVGL Demo 任务: 初始化 + 创建全功能 Demo + 周期调度
  */
void StartLvglTestTask(void *argument)
{
  uint16_t lcd_id;

  dbg_printf("[dbg] lvglTestTask started\r\n");

  dbg_printf("[dbg] LCD_Init ...\r\n");
  LCD_Init();
  lcd_id = LCD_ReadID();
  dbg_printf("[dbg] LCD_Init done, ID=0x%04X\r\n", lcd_id);

  if (GT9147_Init())
  {
    dbg_printf("[dbg] GT9147 OK\r\n");
  }
  else
  {
    dbg_printf("[dbg] GT9147 FAIL\r\n");
  }

  dbg_printf("[dbg] lv_init ...\r\n");
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  dbg_printf("[dbg] LVGL ready\r\n");

  /* ---- 创建 Demo 界面 ---- */
  demo_create();  /* 完整版 Tabview */
  // demo_create_simple();  /* 极简版，测试是否是 UI 复杂度问题 */

  /* ---- 周期调度 ---- */
  uint32_t alive_cnt = 0;
  uint32_t last_tick = HAL_GetTick();
  uint32_t perf_last_tick = last_tick;
  uint32_t max_handler_ms = 0;
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s (LSI 32kHz/16, Reload 4095) */
    uint32_t now = HAL_GetTick();
    lv_tick_inc(now - last_tick);
    last_tick = now;

    if (++alive_cnt % 200 == 0)   /* 5ms x 200 = 1s, 证明循环在跑 */
    {
      dbg_printf("[dbg] alive %lu s\r\n", (unsigned long)(alive_cnt / 200));
    }

    /* 更新 Tab 4 高级特性中的动画控件 */
    demo_advanced_update();

    uint32_t handler_start = HAL_GetTick();
    lv_task_handler();   /* LVGL 任务调度 */
    uint32_t handler_ms = HAL_GetTick() - handler_start;
    if (handler_ms > max_handler_ms) max_handler_ms = handler_ms;

    if ((uint32_t)(HAL_GetTick() - perf_last_tick) >= 1000U)
    {
      uint32_t flush_count;
      uint32_t flush_pixels;
      lv_port_disp_get_stats(&flush_count, &flush_pixels);
      dbg_printf("[perf] lvgl_max=%lums flush=%lu pixels=%lu\r\n",
                 (unsigned long)max_handler_ms,
                 (unsigned long)flush_count,
                 (unsigned long)flush_pixels);
      max_handler_ms = 0;
      perf_last_tick = HAL_GetTick();
    }
    osDelay(5);
  }
}

/* ---- FreeRTOS 调试钩子 (由 configCHECK_FOR_STACK_OVERFLOW=2 / configUSE_MALLOC_FAILED_HOOK=1 触发) ---- */

/* 栈溢出检测 (方法2): 任务栈顶 magic word 被踩 → 打印任务名后卡死, IWDG ~2s 复位重现现场 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  dbg_printf("[HOOK] STACK OVERFLOW task=%s\r\n", pcTaskName);
  __disable_irq();
  while (1) { __WFI(); }
}

/* malloc 失败: pvPortMalloc 返回 NULL */
void vApplicationMallocFailedHook(void)
{
  dbg_printf("[HOOK] MALLOC FAILED\r\n");
  __disable_irq();
  while (1) { __WFI(); }
}

/* USER CODE END Application */

