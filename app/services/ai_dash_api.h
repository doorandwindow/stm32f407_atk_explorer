/**
 ******************************************************************************
 * @file    ai_dash_api.h
 * @brief   板端 DeepSeek 用量仪表盘的数据接口（连 PC 代理拉扁平文本）
 ******************************************************************************
 */

#ifndef AIDASH_API_H
#define AIDASH_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   配置（改这里指向你的 PC 代理）
   ========================================================================== */
#define AIDASH_PROXY_IP      "192.168.31.83"   /* PC 的局域网 IP，见 tools/deepseek_proxy.py */
#define AIDASH_PROXY_PORT    8000              /* 代理端口 */
#define AIDASH_INTERVAL_MS   30000UL           /* 自动刷新间隔（ms） */

/* ============================================================================
   数据契约
   ========================================================================== */
#define AIDASH_SERIES_MAX    31      /* 每天的请求/输入/输出/缓存序列点 */
#define AIDASH_MODEL_MAX     8       /* 模型明细表最大行数 */
#define AIDASH_MODEL_FIELDS  6       /* 模型记录字段数: name:req:tok:out:cache:cost */

typedef struct {
  uint8_t  ok;                      /* 1 = 数据有效 */
  char     bal_cny[16];             /* 余额 CNY */
  char     bal_usd[16];             /* 余额 USD */
  uint8_t  avail;                   /* 余额是否可用 */
  char     today_cost_cny[16];      /* 今日费用 CNY */
  char     today_cost_usd[16];      /* 今日费用 USD */
  char     month_cost_cny[16];      /* 本月费用 CNY */
  uint32_t req_today;               /* 今日请求数 */
  uint32_t tok_today;               /* 今日 token 数 */
  float    cache_hit;               /* 今日缓存命中率 % */
  uint8_t  peak;                    /* 1=当前高峰计费时段 */
  char     update[8];               /* 最近更新时间 HH:MM */

  uint16_t req_series[AIDASH_SERIES_MAX];      /* 每天请求数 */
  uint16_t in_series[AIDASH_SERIES_MAX];       /* 每天输入 token（未缓存） */
  uint16_t out_series[AIDASH_SERIES_MAX];      /* 每天输出 token */
  uint16_t cache_series[AIDASH_SERIES_MAX];    /* 每天缓存命中 token */
  uint16_t series_len;                         /* 序列实际长度 */

  uint16_t model_cnt;                          /* 模型行数 */
  char     model_name[AIDASH_MODEL_MAX][20];
  uint32_t model_req[AIDASH_MODEL_MAX];
  uint32_t model_tok[AIDASH_MODEL_MAX];
  uint32_t model_out[AIDASH_MODEL_MAX];
  float    model_cache[AIDASH_MODEL_MAX];      /* 缓存占比 % */
  char     model_cost[AIDASH_MODEL_MAX][16];   /* 费用 USD */
} ai_dash_data_t;

/**
 * @brief  拉取一次仪表盘数据
 * @param  out: 输出缓冲区（本函数全程喂狗，socket 带超时）
 * @return 0 成功; 负值失败(见实现)。失败时 out 保持原样。
 */
int ai_dash_poll(ai_dash_data_t *out);

/* ============================================================================
   共享数据（由 dashboard_task 写、仪表盘屏读）
   ========================================================================== */
extern ai_dash_data_t g_dash;          /* 最近一次成功拉取的数据 */
extern volatile uint8_t g_dash_ready;  /* 1 = 有新数据待刷新（读后清零） */

/** 请求立即刷新一次（置位 g_dash_poll_now） */
void ai_dash_request_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* AIDASH_API_H */
