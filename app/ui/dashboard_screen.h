/**
 ******************************************************************************
 * @file    dashboard_screen.h
 * @brief   DeepSeek 用量仪表盘独立屏（开机默认屏）
 ******************************************************************************
 */

#ifndef DASHBOARD_SCREEN_H
#define DASHBOARD_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/** 创建仪表盘独立屏，并设为当前屏幕（lv_scr_load） */
void dashboard_screen_create(void);

/** 由 lvgl 循环周期调用：按 g_dash_ready 刷新卡片/图表/表格 */
void dashboard_screen_update(void);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_SCREEN_H */
