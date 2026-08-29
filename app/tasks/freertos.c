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
#include "board.h"
#include "led_pwm.h"
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
/* Definitions for keyLedTask (按键控制 LED 任务) */
osThreadId_t keyLedTaskHandle;
const osThreadAttr_t keyLedTask_attributes = {
  .name = "keyLedTask",
  .stack_size = 1024 * 4,   /* 4KB: 跑 dbg_printf(vsnprintf ~160B 栈缓冲) + HAL 调用链 */
  .priority = (osPriority_t) osPriorityNormal,
};

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

/* Definitions for keyBrightTask (长按调节 LED 亮度任务) */
osThreadId_t keyBrightTaskHandle;
const osThreadAttr_t keyBrightTask_attributes = {
  .name = "keyBrightTask",
  .stack_size = 1024 * 4,   /* 4KB: dbg_printf(vsnprintf ~160B 栈缓冲) + 按键轮询 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartLvglTestTask(void *argument);
void StartKeyLedTask(void *argument);
void StartKeyBrightTask(void *argument);
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
  keyLedTaskHandle = osThreadNew(StartKeyLedTask, NULL, &keyLedTask_attributes);
  keyBrightTaskHandle = osThreadNew(StartKeyBrightTask, NULL, &keyBrightTask_attributes);
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

/**
  * @brief  按键控制 LED 任务: KEY0(PE4) 按下翻转 LED1(PF10)
  * @note   硬件(探索者 V2.2): KEY0=PE4(低有效, 内部上拉), LED1=PF10(低电平点亮)
  *         逻辑: 按下 -> 灯亮; 再按 -> 灯灭 (每次"按下沿"消抖后翻转一次)
  *         本任务自带 GPIO 初始化 + 轮询, 必须喂狗, 防止 >2s 阻塞触发 IWDG
  * @param  argument: Not used
  * @retval None
  */
void StartKeyLedTask(void *argument)
{
  (void)argument;

  /* ---- 引脚时钟与 GPIO 初始化 (CubeMX 未配置 KEY/LED, 此处按 board.h 初始化) ---- */
  __HAL_RCC_GPIOE_CLK_ENABLE();   /* KEY0: GPIOE */
  __HAL_RCC_GPIOF_CLK_ENABLE();   /* LED1: GPIOF */

  /* KEY0 (PE4): 普通输入 + 内部上拉 (松开=高, 按下接地=低) */
  GPIO_InitTypeDef gpio_key = {0};
  gpio_key.Pin = BOARD_KEY0_PIN;
  gpio_key.Mode = GPIO_MODE_INPUT;
  gpio_key.Pull = GPIO_PULLUP;
  gpio_key.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_KEY0_GPIO, &gpio_key);

  /* LED1 (PF10): 推挽输出, 低电平点亮 */
  GPIO_InitTypeDef gpio_led = {0};
  gpio_led.Pin = BOARD_LED1_PIN;
  gpio_led.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_led.Pull = GPIO_NOPULL;
  gpio_led.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_LED1_GPIO, &gpio_led);

  /* 初始状态: 熄灭 */
  uint8_t led_on = 0;
  HAL_GPIO_WritePin(BOARD_LED1_GPIO, BOARD_LED1_PIN, BOARD_LED_INACTIVE_LEVEL);
  dbg_printf("[key] keyLedTask started, KEY0=PE4 LED1=PF10 (active-low)\r\n");

  const uint32_t debounce_ms = 20;   /* 消抖窗口 */
  const uint32_t poll_ms = 10;       /* 轮询周期 */

  uint8_t last_pressed = 0;   /* 上一次是否处于按下, 初始为未按下 */
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */

    uint8_t pressed = (HAL_GPIO_ReadPin(BOARD_KEY0_GPIO, BOARD_KEY0_PIN) == BOARD_KEY0_PRESS_LEVEL);

    /* 检测"按下沿" (松开 -> 按下) */
    if (pressed && !last_pressed)
    {
      /* 消抖: 等 debounce_ms 后再次确认仍处于按下 */
      osDelay(debounce_ms);
      HAL_IWDG_Refresh(&hiwdg);
      if (HAL_GPIO_ReadPin(BOARD_KEY0_GPIO, BOARD_KEY0_PIN) == BOARD_KEY0_PRESS_LEVEL)
      {
        /* 确认是一次有效按下, 翻转 LED 亮灭 */
        led_on = !led_on;
        HAL_GPIO_WritePin(BOARD_LED1_GPIO, BOARD_LED1_PIN,
                          led_on ? BOARD_LED_ACTIVE_LEVEL : BOARD_LED_INACTIVE_LEVEL);
        dbg_printf("[key] KEY0 press -> LED1 %s\r\n", led_on ? "ON" : "OFF");
      }
      /* 等待松开, 避免按住时重复触发; 期间持续喂狗 */
      while (HAL_GPIO_ReadPin(BOARD_KEY0_GPIO, BOARD_KEY0_PIN) == BOARD_KEY0_PRESS_LEVEL)
      {
        HAL_IWDG_Refresh(&hiwdg);
        osDelay(debounce_ms);
      }
    }
    last_pressed = pressed;

    osDelay(poll_ms);
  }
}

/**
  * @brief  按键长按调节 LED 亮度任务: KEY1(PE3) 按住 -> LED0(PF9) 亮度 0%->100%->0% 循环
  * @note   硬件(探索者 V2.2): KEY1=PE3(低有效, 内部上拉), LED0=PF9(低电平点亮)
  *         亮度用 TIM14_CH1 硬件 PWM(1kHz, 1000 级), 本任务只调比较值 set_duty, 无中断
  *         交互: 按住 ≥ 400ms 开始调光; 持续按住 0%->100%->0% 往返; 松开停在当前亮度
  *         本任务自带 GPIO 初始化 + 轮询, 必须喂狗, 防止 >2s 阻塞触发 IWDG
  * @param  argument: Not used
  * @retval None
  */
void StartKeyBrightTask(void *argument)
{
  (void)argument;

  /* ---- KEY1 (PE3): 普通输入 + 内部上拉 (松开=高, 按下接地=低) ---- */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  GPIO_InitTypeDef gpio_key = {0};
  gpio_key.Pin = BOARD_KEY1_PIN;
  gpio_key.Mode = GPIO_MODE_INPUT;
  gpio_key.Pull = GPIO_PULLUP;
  gpio_key.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_KEY1_GPIO, &gpio_key);

  /* ---- LED0 (PF9): 硬件 PWM 调光 (TIM14_CH1, 初始灭) ---- */
  led_pwm_init();
  uint8_t duty = led_pwm_get_duty();
  dbg_printf("[bright] keyBrightTask started, KEY1=PE3 LED0=PF9 (HW-PWM, %d levels)\r\n",
             (int)LED_PWM_LEVELS);

  const uint32_t debounce_ms   = 20;    /* 消抖窗口 */
  const uint32_t longpress_ms  = 400;   /* 长按阈值: 按住 >=400ms 才进入调光 */
  const uint32_t ramp_step_ms  = 20;    /* 每次 20ms 调一级 (2s 扫完 0->100) */
  const uint32_t poll_ms       = 10;    /* 轮询周期 */

  uint8_t last_pressed = 0;   /* 上一次是否处于按下, 初始为未按下 */
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */

    uint8_t pressed = (HAL_GPIO_ReadPin(BOARD_KEY1_GPIO, BOARD_KEY1_PIN) == BOARD_KEY1_PRESS_LEVEL);

    /* 检测"按下沿" (松开 -> 按下) */
    if (pressed && !last_pressed)
    {
      /* 消抖: 等 debounce_ms 后再次确认仍处于按下 */
      osDelay(debounce_ms);
      HAL_IWDG_Refresh(&hiwdg);
      if (HAL_GPIO_ReadPin(BOARD_KEY1_GPIO, BOARD_KEY1_PIN) == BOARD_KEY1_PRESS_LEVEL)
      {
        /* 确认一次有效按下, 进入长按调光循环 */
        uint32_t hold_ms  = 0;
        uint8_t  active   = 0;   /* 是否已跨过长按阈值开始调光 */
        int8_t   dir      = 1;   /* +1 增亮, -1 减亮 (0%->100%->0% 往返) */
        dbg_printf("[bright] KEY1 hold -> start ramping\r\n");

        while (HAL_GPIO_ReadPin(BOARD_KEY1_GPIO, BOARD_KEY1_PIN) == BOARD_KEY1_PRESS_LEVEL)
        {
          HAL_IWDG_Refresh(&hiwdg);          /* 按住期间持续喂狗 */

          hold_ms += ramp_step_ms;
          if (hold_ms >= longpress_ms)
          {
            active = 1;
          }

          if (active)
          {
            if (dir > 0)
            {
              if (duty >= 100U) { duty = 100U; dir = -1; }
              else              { duty++; }
            }
            else
            {
              if (duty <= 0U)   { duty = 0U;   dir = 1;  }
              else              { duty--; }
            }
            led_pwm_set_duty(duty);

            /* 每跨 5% 打印一次, 便于串口观察调光; 避免每 20ms 刷屏 */
            if (duty % 5U == 0U)
            {
              dbg_printf("[bright] duty %u%%\r\n", duty);
            }
          }
          osDelay(ramp_step_ms);
        }

        if (active)
        {
          dbg_printf("[bright] KEY1 release, duty=%u%%\r\n", duty);
        }
        else
        {
          dbg_printf("[bright] KEY1 tap(short) -> no change\r\n");
        }
      }
      /* 注: 上面的 hold 循环在松开时即退出, 无需在此再等松开 */
    }
    last_pressed = pressed;

    osDelay(poll_ms);
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

