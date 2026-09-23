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
#include "uart_dbg.h"
#include "iwdg.h"
#include "smart_tasks.h"
#include "smart_data.h"
#include "esp8266_uart.h"
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
/* 2026-08-30 P1 清场: keyLed/keyBright/dashboard/lvglTest 四任务及其
   demo/服务(demos、dashboard_screen、ai_dash_api、led_pwm)整体退役,
   资源让位 small_smart 移植(Route B)。仅保留 defaultTask(看门狗守护)
   与 monitorTask(内存/水位监控, 移植验收仪器, P6 收尾再撤)。 */
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,   /* 1KB: P2 拔除 LwIP 后本任务只剩喂狗循环(实测峰值 ~100B 量级);
                              旧值 2KB 因当时跑 MX_LWIP_Init(); 512B 溢出是 LwIP 调用链所致,
                              LwIP 已移除, 1KB 留足余量, 水位见 [mon] stk(b) 行 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for monitorTask (系统状态监控任务) */
osThreadId_t monitorTaskHandle;
const osThreadAttr_t monitorTask_attributes = {
  .name = "monitorTask",
  .stack_size = 512 * 4,   /* 2KB: vTaskGetInfo + dbg_printf(vsnprintf ~160B); 堆吃紧(40KB-30KB)从轻 */
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartMonitorTask(void *argument);
void StartEspTask(void *argument);        /* N2 精简版: WiFi/TCP 状态机 + 5s 心跳 */
extern osThreadId_t       espTaskHandle;  /* 定义在 esp_task.c */
extern const osThreadAttr_t espTask_attributes;
extern volatile uint8_t   g_net_status;   /* 0=离线 1=WiFi 2=TCP在线 (UI/mon 只读) */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Private application code --------------------------------------------------*/

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  smart_data_init();   /* small_smart 共享数据模型清零置初值 (任务创建前) */
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
  /* small_smart 移植: UI 任务先起(建屏), 骨架任务随后; 顺序无强依赖 */
  smartUiHandle = osThreadNew(StartSmartUiTask, NULL, &smartUi_attributes);
  smartTickHandle = osThreadNew(StartSmartTickTask, NULL, &smartTick_attributes);
  smartPeriodicHandle = osThreadNew(StartSmartPeriodicTask, NULL, &smartPeriodic_attributes);
  smartAlarmHandle = osThreadNew(StartSmartAlarmTask, NULL, &smartAlarm_attributes);
  monitorTaskHandle = osThreadNew(StartMonitorTask, NULL, &monitorTask_attributes);
  /* N2 (2026-09-23): espTask 常驻上线 - WiFi/TCP 状态机 + 5s 心跳 (probe 已退役) */
  esp8266_uart_init();
  espTaskHandle = osThreadNew(StartEspTask, NULL, &espTask_attributes);
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
  /* P2 (2026-08-30): MX_LWIP_Init() 已随 LwIP+ETH 拔除移除, 本任务现为纯看门狗守护 */
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

