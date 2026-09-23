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

/* lv_port_indev.c 自愈接口 (无头文件, 使用处声明) */
extern uint16_t lv_port_indev_tp_err_run(void);
extern void     lv_port_indev_tp_err_reset(void);

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

  /* GT init 重试: ESP8266 上电连网(EEPROM 自动连)与 GT 上电初始化同窗口,
     3.3V 轨被发射电流拉低会让 GT 带病上岗(2026-09-23 实测: 同树上电触摸必死)。
     间隔 2s 重试躲开模组连网窗口; 喂狗由 defaultTask(1ms) 兜底, 重试期安全 */
  {
    uint8_t gt_ok = 0;
    for (int i = 0; i < 4 && !gt_ok; i++)
    {
      gt_ok = GT9147_Init();
      if (!gt_ok && i < 3)
      {
        dbg_printf("[ui] GT init FAIL, retry in 2s (%d/3)\r\n", i + 1);
        osDelay(2000);
      }
    }
    dbg_printf(gt_ok ? "[ui] GT9147 OK\r\n" : "[ui] GT9147 FAIL (gave up)\r\n");
  }

  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  smart_pages_init();

  uint32_t last_tick = HAL_GetTick();
  uint32_t last_tp_reinit = 0;   /* 自愈节流: 距上次重 init <5s 不再试 */
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */
    uint32_t now = HAL_GetTick();
    lv_tick_inc(now - last_tick);
    last_tick = now;

    /* 触摸自愈: 连续 200 次 I2C 错 (200x5ms=1s 无响应) => GT 被欠压打挂,
       重新走 init (软复位+配置, 幂等保护在驱动内)。
       re-init 含 ~300ms 忙等延时, 期间画面短暂停顿属预期; 5s 节流防风暴 */
    if (lv_port_indev_tp_err_run() >= 200 &&
        (now - last_tp_reinit) > 5000u)
    {
      last_tp_reinit = now;
      dbg_printf("[ui] TP dead (err_run=%u), re-init GT\r\n",
                 (unsigned)lv_port_indev_tp_err_run());
      if (GT9147_Init())
        dbg_printf("[ui] GT re-init OK\r\n");
      else
        dbg_printf("[ui] GT re-init FAIL\r\n");
      lv_port_indev_tp_err_reset();
    }

    smart_pages_update();
    lv_task_handler();
    osDelay(5);
  }
}
