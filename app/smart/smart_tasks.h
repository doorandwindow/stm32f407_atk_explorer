/**
 ******************************************************************************
 * @file    smart_tasks.h
 * @brief   small_smart 移植 - 任务入口统一声明 (供 app/tasks/freertos.c 注册)
 * @note    任务栈/优先级定义在各 .c 文件, 与源工程映射:
 *            源 periodic(RT优先级13) -> smart_periodic (osPriorityNormal, 10ms)
 *            源 alarm   (RT优先级12) -> smart_alarm    (osPriorityNormal, 1s)
 *            源 app_timer 软定时器   -> smart_tick     (BelowNormal, 1s 循环,
 *              不用 osTimer: timer 服务任务优先级只有 2, 回调会被应用任务饿死)
 ******************************************************************************
 */
#ifndef SMART_TASKS_H
#define SMART_TASKS_H

#include <cmsis_os.h>

extern osThreadId_t smartTickHandle;
extern osThreadId_t smartPeriodicHandle;
extern osThreadId_t smartAlarmHandle;
extern osThreadId_t smartUiHandle;

extern const osThreadAttr_t smartTick_attributes;
extern const osThreadAttr_t smartPeriodic_attributes;
extern const osThreadAttr_t smartAlarm_attributes;
extern const osThreadAttr_t smartUi_attributes;

void StartSmartTickTask(void *argument);     /* 1s: 节拍标志 + 桩数据 */
void StartSmartPeriodicTask(void *argument); /* 10ms: 控制状态机(桩) + 1s 消费 */
void StartSmartAlarmTask(void *argument);    /* 5s: 报警巡检 */
void StartSmartUiTask(void *argument);       /* 5ms: LCD/LVGL + 页面管理器 */

#endif /* SMART_TASKS_H */
