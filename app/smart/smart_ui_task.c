/**
 ******************************************************************************
 * @file    smart_ui_task.c
 * @brief   small_smart 移植 - UI 任务 (源 lcd 线程 + 旧 lvglTestTask 合体)
 * @note    初始化链沿旧 lvglTestTask 实证序列: LCD -> GT9147 -> lv_init ->
 *          显示/输入 port -> 页面管理器; 5ms 循环 lv_tick_inc + 页面刷新 +
 *          lv_task_handler + 喂狗。8KB 栈沿用清场前实证值, 水位校准看
 *          [mon] stk(b) 的 ui 项。
 ******************************************************************************
 */
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "main.h"
#include "iwdg.h"
#include "uart_dbg.h"
#include "lcd.h"
#include "gt9147.h"
#include "lvgl.h"
#include "smart_pages.h"

osThreadId_t smartUiHandle;
const osThreadAttr_t smartUi_attributes = {
  .name = "smart_ui",
  .stack_size = 2048 * 4,                  /* 8KB: LVGL 渲染 + 字库格式化 */
  .priority = (osPriority_t) osPriorityNormal,
};

void StartSmartUiTask(void *argument)
{
  (void)argument;

  dbg_printf("[ui] smart_ui started\r\n");
  dbg_printf("[ui] LCD_Init ...\r\n");
  LCD_Init();
  dbg_printf("[ui] LCD ID=0x%04X\r\n", (unsigned)LCD_ReadID());

  if (GT9147_Init())
  {
    dbg_printf("[ui] GT9147 OK\r\n");
  }
  else
  {
    dbg_printf("[ui] GT9147 FAIL\r\n");   /* 触摸挂了也要出画面, 只丢触摸 */
  }

  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  smart_pages_init();

  uint32_t last_tick = HAL_GetTick();
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */
    uint32_t now = HAL_GetTick();
    lv_tick_inc(now - last_tick);
    last_tick = now;

    smart_pages_update();
    lv_task_handler();
    osDelay(5);
  }
}
