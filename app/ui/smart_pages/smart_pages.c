/**
 ******************************************************************************
 * @file    smart_pages.c
 * @brief   small_smart 移植 - 页面管理器实现
 ******************************************************************************
 */
#include "smart_pages.h"
#include "smart_page_main.h"
#include "smart_text.h"
#include "uart_dbg.h"

/* 页面操作表 (update 可为 NULL: 纯静态页) */
typedef struct
{
  void (*create)(lv_obj_t *parent);
  void (*update)(void);
} smart_page_ops_t;

static const smart_page_ops_t page_ops[SMART_PAGE_CNT] = {
  /* [SMART_PAGE_MAIN]   */ { smart_page_main_create,   smart_page_main_update },
  /* [SMART_PAGE_SETUP]  */ { smart_page_setup_create,   NULL },
  /* [SMART_PAGE_ALARMS] */ { smart_page_alarms_create,  smart_page_alarms_update },
  /* [SMART_PAGE_TARTEMP]*/ { smart_page_tartemp_create, smart_page_tartemp_update },
  /* [SMART_PAGE_ABOUT]  */ { smart_page_about_create,   smart_page_about_update },
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

  dbg_printf("[ui] go page %u\r\n", (unsigned)id);   /* 导航验证用 */

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

/* ---- 页面公共工具 ---- */
lv_obj_t *smart_pages_util_title(lv_obj_t *parent, const char *text)
{
  lv_obj_t *title = lv_label_create(parent);
  lv_label_set_text(title, text);
  lv_obj_set_style_text_font(title, &smart_cn_24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(SMART_COL_ACCENT), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 14);
  return title;
}

/* 返回主页: 切页只能在 UI 任务上下文 (lv_task_handler 调度事件回调, 安全) */
static void back_btn_event(lv_event_t *e)
{
  (void)e;
  smart_pages_go(SMART_PAGE_MAIN);
}

lv_obj_t *smart_pages_util_back_btn(lv_obj_t *parent)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 110, 40);
  lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -16, 10);
  lv_obj_set_style_bg_color(btn, lv_color_hex(SMART_COL_CARD), 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(SMART_COL_BORDER), 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_radius(btn, 6, 0);

  lv_obj_t *lb = lv_label_create(btn);
  lv_label_set_text(lb, LV_SYMBOL_LEFT " " TXT_NAV_HOME);
  lv_obj_set_style_text_font(lb, &smart_cn_16, 0);
  lv_obj_set_style_text_color(lb, lv_color_hex(SMART_COL_TXT), 0);
  lv_obj_center(lb);

  lv_obj_add_event_cb(btn, back_btn_event, LV_EVENT_CLICKED, NULL);
  return btn;
}
