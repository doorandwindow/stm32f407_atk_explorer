/**
 ******************************************************************************
 * @file    smart_pages.c
 * @brief   small_smart 移植 - 页面管理器实现
 ******************************************************************************
 */
#include "smart_pages.h"
#include "smart_page_main.h"
#include "uart_dbg.h"

/* 页面操作表 (P5 扩充时在此登记) */
typedef struct
{
  void (*create)(lv_obj_t *parent);
  void (*update)(void);
} smart_page_ops_t;

static const smart_page_ops_t page_ops[SMART_PAGE_CNT] = {
  /* [SMART_PAGE_MAIN] */ { smart_page_main_create, smart_page_main_update },
};

static smart_page_id_t cur_page = SMART_PAGE_MAIN;
static lv_obj_t *page_host = NULL;     /* 当前页容器 (整屏子对象) */

void smart_pages_init(void)
{
  /* 根屏: LVGL 默认活动屏, 常驻不删 */
  lv_obj_t *root = lv_scr_act();
  lv_obj_set_style_bg_color(root, lv_color_hex(0x10141A), 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  cur_page = SMART_PAGE_MAIN;
  page_host = lv_obj_create(root);
  lv_obj_remove_style_all(page_host);
  lv_obj_set_size(page_host, LV_PCT(100), LV_PCT(100));
  if (page_ops[cur_page].create != NULL)
  {
    page_ops[cur_page].create(page_host);
  }
  dbg_printf("[ui] pages ready, main created\r\n");
}

void smart_pages_go(smart_page_id_t id)
{
  if ((uint8_t)id >= (uint8_t)SMART_PAGE_CNT) return;
  if (id == cur_page) return;

  if (page_host != NULL)
  {
    lv_obj_del(page_host);       /* 连同旧页全部控件一并回收 (外部 SRAM 池) */
  }
  cur_page = id;
  page_host = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(page_host);
  lv_obj_set_size(page_host, LV_PCT(100), LV_PCT(100));
  if (page_ops[id].create != NULL)
  {
    page_ops[id].create(page_host);
  }
}

void smart_pages_update(void)
{
  if (page_ops[cur_page].update != NULL)
  {
    page_ops[cur_page].update();
  }
}
