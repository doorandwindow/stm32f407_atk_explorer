/**
 ******************************************************************************
 * @file    dashboard_screen.c
 * @brief   DeepSeek 用量仪表盘独立屏（开机默认屏）
 * @note    LVGL 主题: 深蓝背景 + 卡片风; 标签全英文(板子无中文字体)。
 *          数据来自 g_dash(dashboard_task 轮询写入), 本文件只在 lvgl 任务内
 *          通过 dashboard_screen_update() 更新控件, 保证线程安全。
 *          图例: lv_chart 8.3 无原生堆叠柱状图, 用并排 BAR(Input/Output/Cache)近似。
 ******************************************************************************
 */

#include "dashboard_screen.h"

#include "lvgl.h"
#include "ai_dash_api.h"
#include "demo_main.h"
#include "uart_dbg.h"

#include <string.h>
#include <stdio.h>

extern void demo_advanced_update(void);

#define AIDASH_CHART_POINTS  30

/* ---- 主题色 ---- */
#define COL_BG     0x1E293B
#define COL_CARD   0x334155
#define COL_TEXT   0xF1F5F9
#define COL_SUB    0x94A3B8
#define COL_ACC    0x38BDF8
#define COL_OK     0x22C55E
#define COL_PEAK   0xEF4444
#define COL_WARN   0xF59E0B

/* 柱状图三相色 */
#define COL_IN     0x38BDF8
#define COL_OUT    0x22C55E
#define COL_CACHE  0xA78BFA

/* ---- 静态句柄 ---- */
static lv_obj_t *s_dash_scr = NULL;
static lv_obj_t *s_demo_scr = NULL;
static bool s_is_demo = false;

static lv_obj_t *s_bal = NULL, *s_avail = NULL, *s_today = NULL, *s_month = NULL;
static lv_obj_t *s_req = NULL, *s_tok = NULL, *s_cache = NULL;
static lv_obj_t *s_peak = NULL, *s_update = NULL;
static lv_obj_t *s_ch_req = NULL;   static lv_chart_series_t *s_req_ser = NULL;
static lv_obj_t *s_ch_tok = NULL;   static lv_chart_series_t *s_in_ser = NULL,
                                     *s_out_ser = NULL, *s_cache_ser = NULL;
static lv_obj_t *s_table = NULL;

static inline lv_color_t c(uint32_t v) { return lv_color_hex(v); }

/* ------------------------------------------------------------------------- */
/* 小工具                                                                    */
/* ------------------------------------------------------------------------- */
/* 概览卡: 数值在上(大号)、说明在下(灰), 左对齐 */
static lv_obj_t *mk_card(lv_obj_t *parent, const char *caption, lv_obj_t **valout)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, 142, 72);
  lv_obj_set_style_bg_color(card, c(COL_CARD), 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_pad_all(card, 10, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *val = lv_label_create(card);
  lv_label_set_text(val, "--");
  lv_obj_set_style_text_font(val, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(val, c(COL_TEXT), 0);

  lv_obj_t *cap = lv_label_create(card);
  lv_label_set_text(cap, caption);
  lv_obj_set_style_text_font(cap, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(cap, c(COL_SUB), 0);

  if (valout) *valout = val;
  return card;
}

/* 横向 flex 行 */
static lv_obj_t *mk_row(lv_obj_t *parent)
{
  lv_obj_t *r = lv_obj_create(parent);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_color(r, c(COL_BG), 0);
  lv_obj_set_style_border_width(r, 0, 0);
  lv_obj_set_style_pad_all(r, 0, 0);
  lv_obj_set_style_pad_column(r, 8, 0);
  lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
  return r;
}

/* 小节标题(带左侧强调条) */
static lv_obj_t *mk_section(lv_obj_t *parent, const char *title)
{
  lv_obj_t *sec = lv_obj_create(parent);
  lv_obj_set_size(sec, 460, 24);
  lv_obj_set_style_bg_color(sec, c(COL_BG), 0);
  lv_obj_set_style_border_width(sec, 0, 0);
  lv_obj_set_style_pad_all(sec, 2, 0);
  lv_obj_clear_flag(sec, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(sec, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(sec, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *bar = lv_obj_create(sec);
  lv_obj_set_size(bar, 4, 16);
  lv_obj_set_style_bg_color(bar, c(COL_ACC), 0);
  lv_obj_set_style_radius(bar, 2, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(sec);
  lv_label_set_text(lbl, title);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl, c(COL_TEXT), 0);

  return sec;
}

/* 图例: 色点 + 文字 */
static void mk_legend(lv_obj_t *parent, lv_color_t col, const char *text)
{
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_set_size(dot, 10, 10);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dot, col, 0);
  lv_obj_set_style_border_width(dot, 0, 0);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(parent);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl, c(COL_SUB), 0);
}

/* ------------------------------------------------------------------------- */
/* 图表数据                                                                  */
/* ------------------------------------------------------------------------- */
static uint32_t series_max(const uint16_t *a, uint16_t len)
{
  uint32_t m = 0;
  for (uint16_t i = 0; i < len; ++i) { if (a[i] > m) m = a[i]; }
  return m;
}

static void set_pts(lv_obj_t *ch, lv_chart_series_t *ser, const uint16_t *src, uint16_t len)
{
  if (len > AIDASH_SERIES_MAX) len = AIDASH_SERIES_MAX;
  for (uint16_t i = 0; i < len; ++i)
  {
    lv_chart_set_value_by_id(ch, ser, i, (lv_coord_t)src[i]);
  }
}

static void upd_req_chart(const ai_dash_data_t *d)
{
  uint16_t len = d->series_len > AIDASH_CHART_POINTS ? AIDASH_CHART_POINTS : d->series_len;
  uint32_t mx = series_max(d->req_series, len);
  lv_chart_set_range(s_ch_req, LV_CHART_AXIS_PRIMARY_Y, 0, (lv_coord_t)(mx ? mx : 10));
  set_pts(s_ch_req, s_req_ser, d->req_series, len);
}

static void upd_tok_chart(const ai_dash_data_t *d)
{
  uint16_t len = d->series_len > AIDASH_CHART_POINTS ? AIDASH_CHART_POINTS : d->series_len;
  uint32_t mx = series_max(d->in_series, len);
  uint32_t m2 = series_max(d->out_series, len);
  uint32_t m3 = series_max(d->cache_series, len);
  if (m3 > mx) mx = m3;
  if (m2 > mx) mx = m2;
  lv_chart_set_range(s_ch_tok, LV_CHART_AXIS_PRIMARY_Y, 0, (lv_coord_t)(mx ? mx : 10));
  set_pts(s_ch_tok, s_in_ser, d->in_series, len);
  set_pts(s_ch_tok, s_out_ser, d->out_series, len);
  set_pts(s_ch_tok, s_cache_ser, d->cache_series, len);
}

static void upd_table(const ai_dash_data_t *d)
{
  uint16_t rows = d->model_cnt;
  lv_table_set_row_cnt(s_table, rows + 1);
  for (uint16_t r = 0; r < rows && r < AIDASH_MODEL_MAX; ++r)
  {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%s", d->model_name[r]);
    lv_table_set_cell_value(s_table, r + 1, 0, tmp);
    snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)d->model_req[r]);   lv_table_set_cell_value(s_table, r + 1, 1, tmp);
    snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)d->model_tok[r]);   lv_table_set_cell_value(s_table, r + 1, 2, tmp);
    snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)d->model_out[r]);   lv_table_set_cell_value(s_table, r + 1, 3, tmp);
    snprintf(tmp, sizeof(tmp), "%.1f%%", d->model_cache[r]);             lv_table_set_cell_value(s_table, r + 1, 4, tmp);
    snprintf(tmp, sizeof(tmp), "$%s", d->model_cost[r]);                 lv_table_set_cell_value(s_table, r + 1, 5, tmp);
  }
}

/* ------------------------------------------------------------------------- */
/* 按钮回调                                                                  */
/* ------------------------------------------------------------------------- */
static void on_refresh(lv_event_t *e)
{
  (void)e;
  ai_dash_request_poll();
}

static void on_demo_back(lv_event_t *e)
{
  (void)e;
  lv_scr_load(s_dash_scr);
  s_is_demo = false;
}

static void open_demo(void)
{
  if (s_demo_scr == NULL)
  {
    s_demo_scr = lv_obj_create(NULL);
    lv_scr_load(s_demo_scr);
    demo_create();
    lv_obj_t *back = lv_btn_create(s_demo_scr);
    lv_obj_set_size(back, 96, 32);
    lv_obj_set_style_bg_color(back, c(COL_CARD), 0);
    lv_obj_set_style_radius(back, 8, 0);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "< Dashboard");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, on_demo_back, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(back);
  }
  else
  {
    lv_scr_load(s_demo_scr);
  }
  s_is_demo = true;
}

static void on_demo(lv_event_t *e)
{
  (void)e;
  open_demo();
}

/* ------------------------------------------------------------------------- */
/*  对外接口                                                                  */
/* ------------------------------------------------------------------------- */
void dashboard_screen_create(void)
{
  if (s_dash_scr)
  {
    lv_scr_load(s_dash_scr);
    return;
  }

  s_dash_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_dash_scr, c(COL_BG), 0);

  lv_obj_t *body = lv_obj_create(s_dash_scr);
  lv_obj_set_size(body, 480, 800);
  lv_obj_set_style_bg_color(body, c(COL_BG), 0);
  lv_obj_set_style_border_width(body, 0, 0);
  lv_obj_set_style_pad_all(body, 8, 0);
  lv_obj_set_style_pad_row(body, 8, 0);
  lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  /* ---- 标题 + 状态药丸 ---- */
  lv_obj_t *top = lv_obj_create(body);
  lv_obj_set_size(top, 460, 30);
  lv_obj_set_style_bg_color(top, c(COL_BG), 0);
  lv_obj_set_style_border_width(top, 0, 0);
  lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *title = lv_label_create(top);
  lv_label_set_text(title, "DeepSeek Usage");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, c(COL_TEXT), 0);

  lv_obj_t *status = lv_obj_create(top);
  lv_obj_set_size(status, 120, 26);
  lv_obj_set_style_bg_color(status, c(COL_WARN), 0);
  lv_obj_set_style_radius(status, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(status, 0, 0);
  lv_obj_clear_flag(status, LV_OBJ_FLAG_SCROLLABLE);
  s_peak = lv_label_create(status);
  lv_label_set_text(s_peak, "WAIT");
  lv_obj_set_style_text_font(s_peak, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_peak, c(0x1E293B), 0);
  lv_obj_center(s_peak);

  /* 更新时间(右上角) */
  s_update = lv_label_create(top);
  lv_label_set_text(s_update, "--:--");
  lv_obj_set_style_text_font(s_update, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_update, c(COL_SUB), 0);

  /* ---- 概览卡 2x3 ---- */
  lv_obj_t *r1 = mk_row(body);
  mk_card(r1, "Balance (CNY)", &s_bal);
  mk_card(r1, "Balance (USD)", &s_avail);
  mk_card(r1, "Today Cost", &s_today);

  lv_obj_t *r2 = mk_row(body);
  mk_card(r2, "This Month", &s_month);
  mk_card(r2, "Requests", &s_req);
  mk_card(r2, "Tokens", &s_tok);

  /* ---- 请求折线 ---- */
  mk_section(body, "Requests / Day");
  s_ch_req = lv_chart_create(body);
  lv_obj_set_size(s_ch_req, 460, 140);
  lv_chart_set_type(s_ch_req, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(s_ch_req, AIDASH_CHART_POINTS);
  lv_chart_set_div_line_count(s_ch_req, 4, 4);
  s_req_ser = lv_chart_add_series(s_ch_req, c(COL_ACC), LV_CHART_AXIS_PRIMARY_Y);

  /* ---- Token 并排柱状 + 图例 ---- */
  mk_section(body, "Tokens / Day");
  s_ch_tok = lv_chart_create(body);
  lv_obj_set_size(s_ch_tok, 460, 160);
  lv_chart_set_type(s_ch_tok, LV_CHART_TYPE_BAR);
  lv_chart_set_point_count(s_ch_tok, AIDASH_CHART_POINTS);
  lv_chart_set_div_line_count(s_ch_tok, 4, 4);
  s_in_ser    = lv_chart_add_series(s_ch_tok, c(COL_IN), LV_CHART_AXIS_PRIMARY_Y);
  s_out_ser   = lv_chart_add_series(s_ch_tok, c(COL_OUT), LV_CHART_AXIS_PRIMARY_Y);
  s_cache_ser = lv_chart_add_series(s_ch_tok, c(COL_CACHE), LV_CHART_AXIS_PRIMARY_Y);

  lv_obj_t *legend = mk_row(body);
  mk_legend(legend, c(COL_IN), "Input");
  mk_legend(legend, c(COL_OUT), "Output");
  mk_legend(legend, c(COL_CACHE), "Cache");

  /* 缓存命中率 */
  lv_obj_t *chrow = mk_row(body);
  lv_obj_t *chlbl = lv_label_create(chrow);
  lv_label_set_text(chlbl, "Cache Hit");
  lv_obj_set_style_text_font(chlbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(chlbl, c(COL_SUB), 0);
  s_cache = lv_label_create(chrow);
  lv_label_set_text(s_cache, "--%");
  lv_obj_set_style_text_font(s_cache, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_cache, c(COL_TEXT), 0);

  /* ---- 模型表 ---- */
  mk_section(body, "Model Detail");
  s_table = lv_table_create(body);
  lv_obj_set_size(s_table, 460, 158);
  lv_obj_set_style_text_color(s_table, c(COL_TEXT), 0);
  lv_table_set_col_cnt(s_table, 6);
  lv_table_set_row_cnt(s_table, 1);
  lv_table_set_cell_value(s_table, 0, 0, "Model");
  lv_table_set_cell_value(s_table, 0, 1, "Req");
  lv_table_set_cell_value(s_table, 0, 2, "Tokens");
  lv_table_set_cell_value(s_table, 0, 3, "Out");
  lv_table_set_cell_value(s_table, 0, 4, "Cache%");
  lv_table_set_cell_value(s_table, 0, 5, "Cost");
  lv_table_set_col_width(s_table, 0, 150);
  lv_table_set_col_width(s_table, 1, 46);
  lv_table_set_col_width(s_table, 2, 70);
  lv_table_set_col_width(s_table, 3, 56);
  lv_table_set_col_width(s_table, 4, 60);
  lv_table_set_col_width(s_table, 5, 60);

  /* ---- 按钮 ---- */
  lv_obj_t *btnrow = mk_row(body);
  lv_obj_t *b1 = lv_btn_create(btnrow);
  lv_obj_set_size(b1, 210, 42);
  lv_obj_set_style_bg_color(b1, c(COL_ACC), 0);
  lv_obj_set_style_radius(b1, 10, 0);
  lv_obj_t *bl1 = lv_label_create(b1);
  lv_label_set_text(bl1, "Refresh");
  lv_obj_set_style_text_font(bl1, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(bl1, c(0x0F172A), 0);
  lv_obj_center(bl1);
  lv_obj_add_event_cb(b1, on_refresh, LV_EVENT_CLICKED, NULL);

  lv_obj_t *b2 = lv_btn_create(btnrow);
  lv_obj_set_size(b2, 210, 42);
  lv_obj_set_style_bg_color(b2, c(COL_CARD), 0);
  lv_obj_set_style_radius(b2, 10, 0);
  lv_obj_t *bl2 = lv_label_create(b2);
  lv_label_set_text(bl2, "Demo");
  lv_obj_set_style_text_font(bl2, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(bl2, c(COL_TEXT), 0);
  lv_obj_center(bl2);
  lv_obj_add_event_cb(b2, on_demo, LV_EVENT_CLICKED, NULL);

  lv_scr_load(s_dash_scr);
  dbg_printf("[ui] dashboard screen created\r\n");
}

void dashboard_screen_update(void)
{
  if (s_is_demo)
  {
    if (s_demo_scr != NULL)
    {
      demo_advanced_update();
    }
    return;
  }
  if (!g_dash_ready)
  {
    return;
  }
  const ai_dash_data_t *d = &g_dash;

  lv_label_set_text(s_bal,   d->bal_cny);
  lv_label_set_text(s_avail, d->bal_usd);
  lv_label_set_text(s_today, d->today_cost_cny);
  lv_label_set_text(s_month, d->month_cost_cny);
  lv_label_set_text_fmt(s_req,   "%lu", (unsigned long)d->req_today);
  lv_label_set_text_fmt(s_tok,   "%lu", (unsigned long)d->tok_today);
  lv_label_set_text_fmt(s_cache, "%.1f%%", d->cache_hit);
  lv_label_set_text(s_update, d->update);

  /* 状态药丸: 离线(灰) / 余额不足(橙) / 高峰(红) / 低谷(绿) */
  lv_obj_t *peak_parent = lv_obj_get_parent(s_peak);
  if (d->ok)
  {
    if (d->avail)
    {
      if (d->peak)
      {
        lv_obj_set_style_bg_color(peak_parent, c(COL_PEAK), 0);
        lv_label_set_text(s_peak, "PEAK");
      }
      else
      {
        lv_obj_set_style_bg_color(peak_parent, c(COL_OK), 0);
        lv_label_set_text(s_peak, "OFF-PEAK");
      }
    }
    else
    {
      lv_obj_set_style_bg_color(peak_parent, c(COL_WARN), 0);
      lv_label_set_text(s_peak, "NO BAL");
    }
  }
  else
  {
    lv_obj_set_style_bg_color(peak_parent, c(COL_SUB), 0);
    lv_label_set_text(s_peak, "OFFLINE");
  }

  if (s_ch_req) upd_req_chart(d);
  if (s_ch_tok) upd_tok_chart(d);
  upd_table(d);
  dbg_printf("[ui] dashboard refreshed bal=%s req=%luk tok=%luk\r\n",
             d->bal_cny, (unsigned long)(d->req_today), (unsigned long)(d->tok_today));

  g_dash_ready = 0;
}
