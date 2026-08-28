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
extern void lv_port_indev_init(void);
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
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 0.5s */
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* ---- LVGL 测试界面事件回调 ---- */

/** 按钮点击事件: 计数并刷新按钮文字（验证触摸事件链路） */
static void test_btn_event_cb(lv_event_t *e)
{
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  static uint32_t click_cnt = 0;
  click_cnt++;
  lv_label_set_text_fmt(label, "Clicked: %lu", (unsigned long)click_cnt);
}

/** 屏幕按压事件: 实时显示触摸坐标（验证坐标方向与触摸链路） */
static void test_scr_event_cb(lv_event_t *e)
{
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  lv_indev_t *indev = lv_indev_get_act();
  if (indev != NULL)
  {
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_label_set_text_fmt(label, "Touch: %d, %d", (int)p.x, (int)p.y);
  }
}

/**
  * @brief  LVGL 测试任务: 初始化 + 测试界面 + 周期调度
  *
  * 测试项:
  *  1. 进度条动画        -> 渲染/刷新链路
  *  2. 按钮点击计数      -> 触摸事件链路
  *  3. 触摸坐标实时显示  -> 坐标映射方向
  *  4. 周期计数          -> 定时刷新
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

  /* ---- 测试界面 ---- */
  lv_obj_t *scr = lv_scr_act();
  /* Make the screen a pointer target so touches outside child widgets are
     delivered to the coordinate diagnostic callback. */
  lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E293B), 0);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "LVGL Test");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

  lv_obj_t *sub = lv_label_create(scr);
  lv_label_set_text(sub, "F407 Explorer V2.2\n480x800 NT35510");
  lv_obj_set_style_text_color(sub, lv_color_hex(0x94A3B8), 0);
  lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 90);

  /* 按钮 + 点击计数 */
  lv_obj_t *btn = lv_btn_create(scr);
  lv_obj_set_size(btn, 240, 64);
  lv_obj_center(btn);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Clicked: 0");
  lv_obj_center(btn_label);
  lv_obj_add_event_cb(btn, test_btn_event_cb, LV_EVENT_CLICKED, btn_label);

  /* 触摸坐标实时显示 */
  lv_obj_t *touch_label = lv_label_create(scr);
  lv_label_set_text(touch_label, "Touch: --, --");
  lv_obj_set_style_text_color(touch_label, lv_color_hex(0x38BDF8), 0);
  lv_obj_set_style_text_font(touch_label, &lv_font_montserrat_20, 0);
  lv_obj_align(touch_label, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(scr, test_scr_event_cb, LV_EVENT_PRESSED, touch_label);
  lv_obj_add_event_cb(scr, test_scr_event_cb, LV_EVENT_PRESSING, touch_label);

  /* 进度条动画 */
  lv_obj_t *bar = lv_bar_create(scr);
  lv_obj_set_size(bar, 360, 24);
  lv_bar_set_range(bar, 0, 100);
  lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -120);
  /* LVGL 所有对象默认 CLICKABLE 且事件不冒泡: 按在进度条上时 scr 收不到
     PRESSED/PRESSING, 坐标标签会停更 (2026-08-28 用户实测滑到进度条区域就断)。
     与按钮同样加 EVENT_BUBBLE, 让事件继续冒泡到屏幕回调 */
  lv_obj_add_flag(bar, LV_OBJ_FLAG_EVENT_BUBBLE);

  /* 周期计数 */
  lv_obj_t *tick_label = lv_label_create(scr);
  lv_label_set_text(tick_label, "t: 0");
  lv_obj_set_style_text_color(tick_label, lv_color_hex(0x64748B), 0);
  lv_obj_align(tick_label, LV_ALIGN_BOTTOM_MID, 0, -60);

  /* ---- 周期调度 ---- */
  uint8_t bar_val = 0;
  uint32_t tick_cnt = 0;
  uint32_t tick_div = 0;
  uint32_t alive_cnt = 0;
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s (LSI 偏低) */
    lv_tick_inc(5);

    if (++alive_cnt % 200 == 0)   /* 5ms x 200 = 1s, 证明循环在跑 */
    {
      dbg_printf("[dbg] alive %lu s\r\n", (unsigned long)(alive_cnt / 200));
    }
    bar_val += 2;
    if (bar_val > 100) bar_val = 0;
    lv_bar_set_value(bar, bar_val, LV_ANIM_ON);

    if (++tick_div >= 200)   /* 5ms x 200 = 1s 刷新一次计数 */
    {
      tick_div = 0;
      lv_label_set_text_fmt(tick_label, "t: %lu s", (unsigned long)(++tick_cnt));
    }

    lv_task_handler();   /* LVGL 任务调度 (5ms 周期; 不在此处打日志: 会打满串口拖慢循环) */
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

