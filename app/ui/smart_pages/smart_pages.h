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

/* 页面 ID (P5 全量: 主页 + 4 个次级页) */
typedef enum
{
  SMART_PAGE_MAIN = 0,    /* 环境总览 (默认页, 仿源 Screen_Main1) */
  SMART_PAGE_SETUP,       /* 系统设置菜单 (仿源 Screen_SetUp) */
  SMART_PAGE_ALARMS,      /* 报警列表 (仿源 Screen_AlarmList) */
  SMART_PAGE_TARTEMP,     /* 目标温度设置 (仿源 Screen_TarTempSet) */
  SMART_PAGE_ABOUT,       /* 关于 (仿源 Screen_About) */
  SMART_PAGE_CNT
} smart_page_id_t;

/* 中文字库 (app/ui/fonts/, lv_font_conv 生成) */
extern const lv_font_t smart_cn_16;
extern const lv_font_t smart_cn_24;

/* ---- 页面公共配色与工具 (与主页风格一致) ---- */
#define SMART_COL_BG        0x10141A
#define SMART_COL_CARD      0x1B212B
#define SMART_COL_BORDER    0x2A3340
#define SMART_COL_TXT       0xE8EAED
#define SMART_COL_TXT_DIM   0x9AA3AE
#define SMART_COL_ACCENT    0x4FC3F7
#define SMART_COL_OK        0x66BB6A
#define SMART_COL_ALARM     0xE53935
#define SMART_COL_OFF       0x5A6472

/* 次级页标题 (左上角, 24px) */
lv_obj_t *smart_pages_util_title(lv_obj_t *parent, const char *text);
/* 返回主页按钮 (右上角, 内含事件回调) */
lv_obj_t *smart_pages_util_back_btn(lv_obj_t *parent);

void smart_pages_init(void);                    /* 建根屏 + 进入默认页 */
void smart_pages_go(smart_page_id_t id);        /* 切页 */
void smart_pages_update(void);                  /* 当前页周期刷新 (5ms 循环) */

/* ---- 各页面实现 (smart_page_*.c, 由页面管理器登记调用) ---- */
void smart_page_main_create(lv_obj_t *parent);
void smart_page_main_update(void);
void smart_page_setup_create(lv_obj_t *parent);
void smart_page_alarms_create(lv_obj_t *parent);
void smart_page_alarms_update(void);
void smart_page_tartemp_create(lv_obj_t *parent);
void smart_page_tartemp_update(void);
void smart_page_about_create(lv_obj_t *parent);
void smart_page_about_update(void);

#endif /* SMART_PAGES_H */
