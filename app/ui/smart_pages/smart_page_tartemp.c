/**
 ******************************************************************************
 * @file    smart_page_tartemp.c
 * @brief   small_smart 移植 - 目标温度设置页 (仿源 Screen_TarTempSet/Enter)
 * @note    +/- 按钮步进 0.5°C, 范围 5.0~45.0°C, 直接写 g_tar_temp
 *          (0.1°C 定点, volatile int16 原子写); 控制任务下一拍即生效。
 ******************************************************************************
 */
#include "lvgl.h"
#include "smart_pages.h"
#include "smart_text.h"
#include "smart_data.h"
#include <stdio.h>
#include <stdbool.h>

#define TAR_STEP     5      /* 0.5°C (0.1 定点) */
#define TAR_MIN      50     /*  5.0°C */
#define TAR_MAX      450    /* 45.0°C */

typedef struct
{
  lv_obj_t *val_label;
  lv_obj_t *avr_label;
  int16_t   cached_tar;
  int16_t   cached_avr;
} tartemp_ui_t;

static tartemp_ui_t ui;

static void step_event(lv_event_t *e)
{
  int16_t dir = (int16_t)(intptr_t)lv_event_get_user_data(e);
  int16_t v = (int16_t)g_tar_temp + (int16_t)(dir * TAR_STEP);
  if (v < TAR_MIN) { v = TAR_MIN; }
  if (v > TAR_MAX) { v = TAR_MAX; }
  g_tar_temp = v;
}

/* 大号 +/- 按钮 */
static lv_obj_t *make_step_btn(lv_obj_t *parent, const char *sym,
                               lv_align_t align, lv_coord_t x_ofs, int16_t dir)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 130, 130);
  lv_obj_align(btn, align, x_ofs, 40);
  lv_obj_set_style_bg_color(btn, lv_color_hex(SMART_COL_CARD), 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(SMART_COL_ACCENT), 0);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_radius(btn, 12, 0);

  lv_obj_t *lb = lv_label_create(btn);
  lv_label_set_text(lb, sym);
  lv_obj_set_style_text_font(lb, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lb, lv_color_hex(SMART_COL_ACCENT), 0);
  lv_obj_center(lb);

  lv_obj_add_event_cb(btn, step_event, LV_EVENT_CLICKED, (void *)(intptr_t)dir);
  return btn;
}

void smart_page_tartemp_create(lv_obj_t *parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_hex(SMART_COL_BG), 0);
  smart_pages_util_title(parent, TXT_TAR_TEMP_SET);
  smart_pages_util_back_btn(parent);

  ui.val_label = lv_label_create(parent);
  lv_obj_set_style_text_font(ui.val_label, &smart_cn_24, 0);
  lv_obj_set_style_text_color(ui.val_label, lv_color_hex(SMART_COL_TXT), 0);
  lv_obj_align(ui.val_label, LV_ALIGN_TOP_MID, 0, 130);

  lv_obj_t *unit = lv_label_create(parent);
  lv_label_set_text(unit, TXT_UNIT_C " " TXT_TAR_TEMP);
  lv_obj_set_style_text_font(unit, &smart_cn_16, 0);
  lv_obj_set_style_text_color(unit, lv_color_hex(SMART_COL_TXT_DIM), 0);
  lv_obj_align(unit, LV_ALIGN_TOP_MID, 0, 175);

  make_step_btn(parent, "-", LV_ALIGN_TOP_MID, -140, -1);
  make_step_btn(parent, "+", LV_ALIGN_TOP_MID, 140, 1);

  ui.avr_label = lv_label_create(parent);
  lv_obj_set_style_text_font(ui.avr_label, &smart_cn_16, 0);
  lv_obj_set_style_text_color(ui.avr_label, lv_color_hex(SMART_COL_TXT_DIM), 0);
  lv_obj_align(ui.avr_label, LV_ALIGN_TOP_MID, 0, 300);

  ui.cached_tar = -1;
  ui.cached_avr = -1;
}

void smart_page_tartemp_update(void)
{
  char buf[12];
  int16_t v = (int16_t)g_tar_temp;
  if (ui.cached_tar != v)
  {
    ui.cached_tar = v;
    int16_t a = (v < 0) ? -v : v;
    snprintf(buf, sizeof(buf), "%s%d.%d", (v < 0) ? "-" : "", a / 10, a % 10);
    lv_label_set_text(ui.val_label, buf);
  }

  int16_t avr = (int16_t)g_smart_data.t_avr;
  if (ui.cached_avr != avr)
  {
    ui.cached_avr = avr;
    int16_t a = (avr < 0) ? -avr : avr;
    snprintf(buf, sizeof(buf), "%s %s%d.%d", TXT_TEMP_AVR,
             (avr < 0) ? "-" : "", a / 10, a % 10);
    lv_label_set_text(ui.avr_label, buf);
  }
}
