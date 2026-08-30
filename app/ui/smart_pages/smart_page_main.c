/**
 ******************************************************************************
 * @file    smart_page_main.c
 * @brief   small_smart 移植 - 环境总览页 (仿源 Screen_Main1, 横屏 800x480)
 * @note    布局: 顶部标题+报警横幅 | 左侧 2x3 数据卡 | 右侧 2x4 设备状态格。
 *          数据源 g_smart_data/g_smart_state/g_alarm_all (单写多读, UI 只读),
 *          update() 做变化检测, 值不变不重绘, 避免 5ms 循环里空刷。
 ******************************************************************************
 */
#include "smart_page_main.h"
#include "smart_pages.h"
#include "smart_text.h"
#include "smart_data.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* ---- 配色 ---- */
#define COL_BG        0x10141A
#define COL_CARD      0x1B212B
#define COL_BORDER    0x2A3340
#define COL_TXT       0xE8EAED
#define COL_TXT_DIM   0x9AA3AE
#define COL_ACCENT    0x4FC3F7
#define COL_OK        0x66BB6A
#define COL_ALARM     0xE53935
#define COL_OFF       0x5A6472

/* ---- 控件句柄 ---- */
typedef struct
{
  lv_obj_t *banner;         /* 顶部报警横幅 */
  lv_obj_t *banner_label;
  lv_obj_t *t_in;           /* 平均温度大字 */
  lv_obj_t *t_in_sub;       /* 三路小字 */
  lv_obj_t *t_out;
  lv_obj_t *humi;
  lv_obj_t *co2;
  lv_obj_t *pa;
  lv_obj_t *tar;
  lv_obj_t *age;
  lv_obj_t *dev_chip[7];    /* 设备格: 边框状态色 */
  lv_obj_t *dev_val[7];     /* 运行/停止 或 台数/开度 */
  lv_obj_t *dev_ico[7];
} main_ui_t;

static main_ui_t ui;

/* 变化检测缓存 (上次刷到 UI 的值) */
static struct
{
  int16_t t_avr, t1, t2, t3, t_out, tar;
  uint16_t humi, co2, pa;
  uint16_t age;
  uint8_t alarm_all;
  uint8_t dev[7];
} cache;

/* ---- 小工具 ---- */

/* 0.1 定点 -> "x.y" (负数安全, buf>=12B) */
static void fmt_tenths(char *buf, int16_t v)
{
  int16_t a = (v < 0) ? -v : v;
  snprintf(buf, 12, "%s%d.%d", (v < 0) ? "-" : "", a / 10, a % 10);
}

static lv_obj_t *make_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                           lv_coord_t w, lv_coord_t h, const char *title)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_set_style_bg_color(card, lv_color_hex(COL_CARD), 0);
  lv_obj_set_style_border_color(card, lv_color_hex(COL_BORDER), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_font(t, &smart_cn_16, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(COL_TXT_DIM), 0);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 10, 6);
  return card;
}

static lv_obj_t *make_value_label(lv_obj_t *card, const char *unit)
{
  lv_obj_t *l = lv_label_create(card);
  lv_obj_set_style_text_font(l, &smart_cn_24, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(COL_TXT), 0);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 12, 6);
  if (unit != NULL)
  {
    lv_obj_t *u = lv_label_create(card);
    lv_label_set_text(u, unit);
    lv_obj_set_style_text_font(u, &smart_cn_16, 0);
    lv_obj_set_style_text_color(u, lv_color_hex(COL_TXT_DIM), 0);
    lv_obj_align_to(u, l, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, 0);
  }
  return l;
}

/* ---- 设备格 ---- */
#define DEV_FAN   0
#define DEV_WC    1
#define DEV_VFD   2
#define DEV_HEAT  3
#define DEV_SPRAY 4
#define DEV_WD    5
#define DEV_LIGHT 6

static const char *dev_names[7] = {
  TXT_DEV_FAN, TXT_DEV_WC, TXT_DEV_VFD, TXT_DEV_HEAT,
  TXT_DEV_SPRAY, TXT_DEV_WD, TXT_DEV_LIGHT,
};
static const char *dev_icons[7] = {
  LV_SYMBOL_SETTINGS, LV_SYMBOL_TINT, LV_SYMBOL_DRIVE, LV_SYMBOL_CHARGE,
  LV_SYMBOL_LOOP, LV_SYMBOL_EYE_OPEN, LV_SYMBOL_EYE_OPEN,
};

static void make_dev_chip(lv_obj_t *parent, uint8_t idx, lv_coord_t x, lv_coord_t y)
{
  lv_obj_t *chip = lv_obj_create(parent);
  lv_obj_set_pos(chip, x, y);
  lv_obj_set_size(chip, 158, 88);
  lv_obj_set_style_bg_color(chip, lv_color_hex(COL_CARD), 0);
  lv_obj_set_style_border_color(chip, lv_color_hex(COL_OFF), 0);
  lv_obj_set_style_border_width(chip, 2, 0);
  lv_obj_set_style_radius(chip, 8, 0);
  lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
  ui.dev_chip[idx] = chip;

  lv_obj_t *ico = lv_label_create(chip);
  lv_label_set_text(ico, dev_icons[idx]);
  lv_obj_set_style_text_font(ico, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(ico, lv_color_hex(COL_OFF), 0);
  lv_obj_align(ico, LV_ALIGN_TOP_LEFT, 10, 8);
  ui.dev_ico[idx] = ico;

  lv_obj_t *name = lv_label_create(chip);
  lv_label_set_text(name, dev_names[idx]);
  lv_obj_set_style_text_font(name, &smart_cn_16, 0);
  lv_obj_set_style_text_color(name, lv_color_hex(COL_TXT_DIM), 0);
  lv_obj_align(name, LV_ALIGN_TOP_RIGHT, -10, 10);

  lv_obj_t *val = lv_label_create(chip);
  lv_label_set_text(val, TXT_STATE_OFF);
  lv_obj_set_style_text_font(val, &smart_cn_16, 0);
  lv_obj_set_style_text_color(val, lv_color_hex(COL_OFF), 0);
  lv_obj_align(val, LV_ALIGN_BOTTOM_LEFT, 10, -8);
  ui.dev_val[idx] = val;
}

/* ---- 页面创建 ---- */
void smart_page_main_create(lv_obj_t *parent)
{
  memset(&ui, 0, sizeof(ui));
  memset(&cache, 0, sizeof(cache));

  /* 顶部栏: 标题 + 报警横幅 */
  lv_obj_t *title = lv_label_create(parent);
  lv_label_set_text(title, TXT_MAIN_TITLE);
  lv_obj_set_style_text_font(title, &smart_cn_24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(COL_ACCENT), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 14);

  ui.banner = lv_obj_create(parent);
  lv_obj_set_size(ui.banner, 200, 36);
  lv_obj_align(ui.banner, LV_ALIGN_TOP_RIGHT, -16, 10);
  lv_obj_set_style_radius(ui.banner, 6, 0);
  lv_obj_set_style_bg_color(ui.banner, lv_color_hex(COL_OK), 0);
  lv_obj_set_style_border_width(ui.banner, 0, 0);
  lv_obj_clear_flag(ui.banner, LV_OBJ_FLAG_SCROLLABLE);
  ui.banner_label = lv_label_create(ui.banner);
  lv_obj_set_style_text_font(ui.banner_label, &smart_cn_16, 0);
  lv_obj_set_style_text_color(ui.banner_label, lv_color_hex(0x10141A), 0);
  lv_obj_center(ui.banner_label);

  /* ---- 左侧数据卡 2x3, 每张 210x126, 起点 (16, 60) ---- */
  lv_obj_t *c;

  c = make_card(parent, 16, 60, 210, 126, TXT_TEMP_AVR);
  ui.t_in = make_value_label(c, TXT_UNIT_C);
  lv_obj_t *sub = lv_label_create(c);
  lv_obj_set_style_text_font(sub, &smart_cn_16, 0);
  lv_obj_set_style_text_color(sub, lv_color_hex(COL_TXT_DIM), 0);
  lv_obj_align(sub, LV_ALIGN_BOTTOM_LEFT, 12, -6);
  ui.t_in_sub = sub;

  c = make_card(parent, 234, 60, 210, 126, TXT_TEMP_OUT);
  ui.t_out = make_value_label(c, TXT_UNIT_C);

  c = make_card(parent, 16, 194, 210, 126, TXT_HUMI);
  ui.humi = make_value_label(c, TXT_UNIT_HUMI);

  c = make_card(parent, 234, 194, 210, 126, TXT_CO2);
  ui.co2 = make_value_label(c, TXT_UNIT_CO2);

  c = make_card(parent, 16, 328, 210, 126, TXT_PA);
  ui.pa = make_value_label(c, TXT_UNIT_PA);

  c = make_card(parent, 234, 328, 210, 126, TXT_TAR_TEMP);
  ui.tar = make_value_label(c, TXT_UNIT_C);
  lv_obj_t *age = lv_label_create(c);
  lv_obj_set_style_text_font(age, &smart_cn_16, 0);
  lv_obj_set_style_text_color(age, lv_color_hex(COL_TXT_DIM), 0);
  lv_obj_align(age, LV_ALIGN_BOTTOM_LEFT, 12, -6);
  ui.age = age;

  /* ---- 右侧设备格 2x4, 每格 158x88, 起点 (464, 60) ---- */
  for (uint8_t i = 0; i < 4; i++)
  {
    make_dev_chip(parent, i, (lv_coord_t)(464 + (i % 2) * 166),
                  (lv_coord_t)(60 + (i / 2) * 96));
  }
  for (uint8_t i = 4; i < 7; i++)
  {
    make_dev_chip(parent, i, (lv_coord_t)(464 + (i % 2) * 166),
                  (lv_coord_t)(60 + (i / 2) * 96));
  }
}

/* ---- 周期刷新 (5ms 循环, 变化检测) ---- */
void smart_page_main_update(void)
{
  char buf[16];
  const smart_data_t *d = (const smart_data_t *)&g_smart_data;
  const smart_state_t *s = (const smart_state_t *)&g_smart_state;
  bool dirty = false;

  /* 报警横幅 */
  if (cache.alarm_all != g_alarm_all)
  {
    cache.alarm_all = g_alarm_all;
    if (g_alarm_all != 0U)
    {
      lv_obj_set_style_bg_color(ui.banner, lv_color_hex(COL_ALARM), 0);
      lv_label_set_text(ui.banner_label, TXT_ALARM_BANNER);
    }
    else
    {
      lv_obj_set_style_bg_color(ui.banner, lv_color_hex(COL_OK), 0);
      lv_label_set_text(ui.banner_label, TXT_ALARM_NONE);
    }
    dirty = true;
  }

  /* 平均温度 + 三路小字 */
  if (cache.t_avr != d->t_avr)
  {
    cache.t_avr = d->t_avr;
    fmt_tenths(buf, d->t_avr);
    lv_label_set_text(ui.t_in, buf);
    dirty = true;
  }
  if (cache.t1 != d->temp[0] || cache.t2 != d->temp[1] || cache.t3 != d->temp[2])
  {
    cache.t1 = d->temp[0]; cache.t2 = d->temp[1]; cache.t3 = d->temp[2];
    /* 一次性拼三路温度 */
    char b1[8], b2[8], b3[8];
    fmt_tenths(b1, d->temp[0]); fmt_tenths(b2, d->temp[1]); fmt_tenths(b3, d->temp[2]);
    lv_label_set_text_fmt(ui.t_in_sub, "1:%s  2:%s  3:%s", b1, b2, b3);
    dirty = true;
  }
  if (cache.t_out != d->temp_out)
  {
    cache.t_out = d->temp_out;
    fmt_tenths(buf, d->temp_out);
    lv_label_set_text(ui.t_out, buf);
    dirty = true;
  }
  if (cache.humi != d->humi)
  {
    cache.humi = d->humi;
    snprintf(buf, sizeof(buf), "%u.%u", d->humi / 10U, d->humi % 10U);
    lv_label_set_text(ui.humi, buf);
    dirty = true;
  }
  if (cache.co2 != d->co2)
  {
    cache.co2 = d->co2;
    snprintf(buf, sizeof(buf), "%u", d->co2);
    lv_label_set_text(ui.co2, buf);
    dirty = true;
  }
  if (cache.pa != d->pa)
  {
    cache.pa = d->pa;
    snprintf(buf, sizeof(buf), "%u.%u", d->pa / 10U, d->pa % 10U);
    lv_label_set_text(ui.pa, buf);
    dirty = true;
  }
  if (cache.tar != g_tar_temp)
  {
    cache.tar = g_tar_temp;
    fmt_tenths(buf, g_tar_temp);
    lv_label_set_text(ui.tar, buf);
    dirty = true;
  }
  if (cache.age != g_age_days)
  {
    cache.age = g_age_days;
    lv_label_set_text_fmt(ui.age, "%s %u %s", TXT_AGE_DAYS, (unsigned)g_age_days, TXT_UNIT_DAY);
    dirty = true;
  }

  /* 设备状态: 0=停(灰) 1=运行(绿) */
  uint8_t dev_now[7];
  dev_now[DEV_FAN]   = (uint8_t)(s->fan[0] + s->fan[1] + s->fan[2] + s->fan[3]);
  dev_now[DEV_WC]    = s->wc[0];
  dev_now[DEV_VFD]   = s->vfd[0];
  dev_now[DEV_HEAT]  = s->heat[0];
  dev_now[DEV_SPRAY] = s->spray[0];
  dev_now[DEV_WD]    = (s->wd_pos[0] > 0U) ? 1U : 0U;
  dev_now[DEV_LIGHT] = s->light[0];

  for (uint8_t i = 0; i < 7; i++)
  {
    if (cache.dev[i] != dev_now[i])
    {
      cache.dev[i] = dev_now[i];
      lv_color_t col = (dev_now[i] != 0U) ? lv_color_hex(COL_OK) : lv_color_hex(COL_OFF);
      lv_obj_set_style_border_color(ui.dev_chip[i], col, 0);
      lv_obj_set_style_text_color(ui.dev_ico[i], col, 0);
      lv_obj_set_style_text_color(ui.dev_val[i], col, 0);
      if (i == DEV_FAN)
      {
        lv_label_set_text_fmt(ui.dev_val[i], "%u/4 %s", (unsigned)dev_now[i], TXT_STATE_ON);
      }
      else if (i == DEV_WD)
      {
        lv_label_set_text_fmt(ui.dev_val[i], "%u%%", (unsigned)s->wd_pos[0]);
      }
      else if (i == DEV_VFD)
      {
        lv_label_set_text_fmt(ui.dev_val[i], "%u.%u%%", (unsigned)(s->vfd_speed[0] / 10U),
                              (unsigned)(s->vfd_speed[0] % 10U));
      }
      else
      {
        lv_label_set_text(ui.dev_val[i], (dev_now[i] != 0U) ? TXT_STATE_ON : TXT_STATE_OFF);
      }
      dirty = true;
    }
  }

  (void)dirty;   /* 变化检测已逐项落盘, 预留给整体失效策略 */
}
