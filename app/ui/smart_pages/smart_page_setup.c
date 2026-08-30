/**
 ******************************************************************************
 * @file    smart_page_setup.c
 * @brief   small_smart 移植 - 系统设置菜单页 (仿源 Screen_SetUp, 精简 3 项)
 ******************************************************************************
 */
#include "lvgl.h"
#include "smart_pages.h"
#include "smart_text.h"

/* 菜单按钮统一事件: user_data 携带目标页 ID */
static void menu_btn_event(lv_event_t *e)
{
  smart_pages_go((smart_page_id_t)(uintptr_t)lv_event_get_user_data(e));
}

static lv_obj_t *make_menu_btn(lv_obj_t *parent, const char *text,
                               smart_page_id_t target, lv_coord_t y)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 320, 76);
  lv_obj_set_pos(btn, 240, y);
  lv_obj_set_style_bg_color(btn, lv_color_hex(SMART_COL_CARD), 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(SMART_COL_BORDER), 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_radius(btn, 8, 0);

  lv_obj_t *lb = lv_label_create(btn);
  lv_label_set_text(lb, text);
  lv_obj_set_style_text_font(lb, &smart_cn_16, 0);
  lv_obj_set_style_text_color(lb, lv_color_hex(SMART_COL_TXT), 0);
  lv_obj_center(lb);

  lv_obj_t *arrow = lv_label_create(btn);
  lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(arrow, lv_color_hex(SMART_COL_TXT_DIM), 0);
  lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -14, 0);

  lv_obj_add_event_cb(btn, menu_btn_event, LV_EVENT_CLICKED, (void *)(uintptr_t)target);
  return btn;
}

void smart_page_setup_create(lv_obj_t *parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_hex(SMART_COL_BG), 0);
  smart_pages_util_title(parent, TXT_SETUP_TITLE);
  smart_pages_util_back_btn(parent);

  make_menu_btn(parent, TXT_TAR_TEMP_SET, SMART_PAGE_TARTEMP, 90);
  make_menu_btn(parent, TXT_ALARM_TITLE, SMART_PAGE_ALARMS, 186);
  make_menu_btn(parent, TXT_ABOUT_TITLE, SMART_PAGE_ABOUT, 282);
}
