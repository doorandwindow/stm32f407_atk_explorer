/**
 ******************************************************************************
 * @file    smart_pages.h
 * @brief   small_smart 移植 - LVGL 页面管理器 (替代源工程 ScrIndex[] 页面协程)
 * @note    模式: 仿 dashboard_screen 的 create/update —— 每页提供
 *            create(parent): 一次性建控件
 *            update():       周期刷新(5ms 循环里被调, 内部做变化检测)
 *          页面切换 = 删除旧页对象 + 创建新页 (LVGL 对象池在外部 SRAM,
 *          128KB 足够全页面缓存, 先按需创建, 不做预建优化)。
 *          中文字库: smart_cn_16/24 (app/ui/fonts/, SimHei 子集 75 字),
 *          图标符号走 montserrat_20 回退链。
 ******************************************************************************
 */
#ifndef SMART_PAGES_H
#define SMART_PAGES_H

#include "lvgl.h"

/* 页面 ID (P5 扩充: SETUP/ALARMS/TARTEMP/ABOUT) */
typedef enum
{
  SMART_PAGE_MAIN = 0,   /* 环境总览 (默认页, 仿源 Screen_Main1) */
  SMART_PAGE_CNT
} smart_page_id_t;

/* 中文字库 (app/ui/fonts/, lv_font_conv 生成) */
extern const lv_font_t smart_cn_16;
extern const lv_font_t smart_cn_24;

void smart_pages_init(void);                    /* 建根屏 + 进入默认页 */
void smart_pages_go(smart_page_id_t id);        /* 切页 */
void smart_pages_update(void);                  /* 当前页周期刷新 (5ms 循环) */

#endif /* SMART_PAGES_H */
