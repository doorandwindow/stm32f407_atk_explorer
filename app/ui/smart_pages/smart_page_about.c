/**
 ******************************************************************************
 * @file    smart_page_about.c
 * @brief   small_smart 移植 - 关于页 (仿源 Screen_About)
 * @note    静态信息 + 运行时长 (软时钟 Timer_SecNow, 1s 刷新一次显示)。
 ******************************************************************************
 */
#include "lvgl.h"
#include "smart_pages.h"
#include "smart_text.h"
#include "smart_data.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct
{
  lv_obj_t *uptime_label;
  uint32_t  cached_sec;
} about_ui_t;

static about_ui_t ui;

static lv_obj_t *make_info_row(lv_obj_t *parent, lv_coord_t y,
                               const char *key, const char *val)
{
  lv_obj_t *k = lv_label_create(parent);
  lv_label_set_text(k, key);
  lv_obj_set_style_text_font(k, &smart_cn_16, 0);
  lv_obj_set_style_text_color(k, lv_color_hex(SMART_COL_TXT_DIM), 0);
  lv_obj_set_pos(k, 240, y);

  lv_obj_t *v = lv_label_create(parent);
  lv_label_set_text(v, val);
  lv_obj_set_style_text_font(v, &smart_cn_16, 0);
  lv_obj_set_style_text_color(v, lv_color_hex(SMART_COL_TXT), 0);
  lv_obj_set_pos(v, 420, y);
  return v;
}

void smart_page_about_create(lv_obj_t *parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_hex(SMART_COL_BG), 0);
  smart_pages_util_title(parent, TXT_ABOUT_TITLE);
  smart_pages_util_back_btn(parent);

  (void)make_info_row(parent, 110, TXT_ABOUT_CHIP, "STM32F407ZGT6 @168MHz");
  (void)make_info_row(parent, 160, TXT_ABOUT_VER, TXT_VER);
  ui.uptime_label = make_info_row(parent, 210, TXT_MAIN_TITLE, "-");

  ui.cached_sec = 0xFFFFFFFFu;
}

void smart_page_about_update(void)
{
  uint32_t sec = Timer_SecNow;
  if (ui.cached_sec != sec)
  {
    ui.cached_sec = sec;
    lv_label_set_text_fmt(ui.uptime_label, "%us", (unsigned)sec);
  }
}
