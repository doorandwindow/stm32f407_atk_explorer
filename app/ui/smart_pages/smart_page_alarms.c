/**
 ******************************************************************************
 * @file    smart_page_alarms.c
 * @brief   small_smart 移植 - 报警列表页 (仿源 Screen_AlarmList)
 * @note    数据源 g_alarm_list (smart_alarm 任务 5s 巡检维护), UI 只读。
 *          已映射文案的报警 3 项, 各占一行; 只显示报警中的项, 全无时居中
 *          显示"无报警"。update() 变化检测重建列表。
 ******************************************************************************
 */
#include "lvgl.h"
#include "smart_pages.h"
#include "smart_text.h"
#include "smart_data.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* 报警编号 -> 文案 (编号定义见 smart_data.h) */
static const char *alm_text[3] = {
  TXT_ALARM_HIGH_TEMP, TXT_ALARM_LOW_TEMP, TXT_ALARM_SENS_FAULT,
};

typedef struct
{
  lv_obj_t *row[3];      /* 每个已知报警一行 */
  lv_obj_t *txt[3];
  lv_obj_t *sec[3];
  lv_obj_t *none_label;
  uint8_t   cached_active[3];
  uint32_t  cached_sec[3];
  uint8_t   cached_any;
} alarm_ui_t;

static alarm_ui_t ui;

void smart_page_alarms_create(lv_obj_t *parent)
{
  memset(&ui, 0, sizeof(ui));
  lv_obj_set_style_bg_color(parent, lv_color_hex(SMART_COL_BG), 0);
  smart_pages_util_title(parent, TXT_ALARM_TITLE);
  smart_pages_util_back_btn(parent);

  for (uint8_t i = 0; i < 3; i++)
  {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 620, 56);
    lv_obj_set_pos(row, 90, (lv_coord_t)(96 + i * 72));
    lv_obj_set_style_bg_color(row, lv_color_hex(SMART_COL_CARD), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);   /* 初始全隐, 由 update 按状态显 */
    ui.row[i] = row;

    lv_obj_t *t = lv_label_create(row);
    lv_label_set_text(t, alm_text[i]);
    lv_obj_set_style_text_font(t, &smart_cn_16, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(SMART_COL_ALARM), 0);
    lv_obj_align(t, LV_ALIGN_LEFT_MID, 16, 0);
    ui.txt[i] = t;

    lv_obj_t *s = lv_label_create(row);
    lv_obj_set_style_text_font(s, &smart_cn_16, 0);
    lv_obj_set_style_text_color(s, lv_color_hex(SMART_COL_TXT_DIM), 0);
    lv_obj_align(s, LV_ALIGN_RIGHT_MID, -16, 0);
    ui.sec[i] = s;
  }

  ui.none_label = lv_label_create(parent);
  lv_label_set_text(ui.none_label, TXT_ALARM_NONE);
  lv_obj_set_style_text_font(ui.none_label, &smart_cn_24, 0);
  lv_obj_set_style_text_color(ui.none_label, lv_color_hex(SMART_COL_TXT_DIM), 0);
  lv_obj_center(ui.none_label);
}

void smart_page_alarms_update(void)
{
  bool changed = (ui.cached_any != g_alarm_all);
  for (uint8_t i = 0; i < 3; i++)
  {
    const smart_alarm_t *a = (const smart_alarm_t *)&g_alarm_list[i];
    if (ui.cached_active[i] != a->active) { changed = true; }
    else if ((a->active != 0U) && (ui.cached_sec[i] != a->sec)) { changed = true; }
  }
  if (!changed) return;

  uint8_t any = 0;
  for (uint8_t i = 0; i < 3; i++)
  {
    const smart_alarm_t *a = (const smart_alarm_t *)&g_alarm_list[i];
    ui.cached_active[i] = a->active;
    ui.cached_sec[i]    = a->sec;

    if (a->active != 0U)
    {
      any = 1;
      char buf[24];
      snprintf(buf, sizeof(buf), "%s %us", TXT_ALARM_TIME, (unsigned)a->sec);
      lv_label_set_text(ui.sec[i], buf);
      lv_obj_clear_flag(ui.row[i], LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
      lv_obj_add_flag(ui.row[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  ui.cached_any = any;
  if (any != 0U)
  {
    lv_obj_add_flag(ui.none_label, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_clear_flag(ui.none_label, LV_OBJ_FLAG_HIDDEN);
  }
}
