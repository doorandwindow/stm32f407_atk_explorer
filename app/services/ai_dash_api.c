/**
 ******************************************************************************
 * @file    ai_dash_api.c
 * @brief   板端 DeepSeek 用量仪表盘数据接口实现
 *
 *  网络要点:
 *    - LwIP `LWIP_DNS=0` -> 只能按 IP 直连代理(方案即如此)。
 *    - 纯 DHCP, 需等 `gnetif.ip_addr.addr != 0` 再连(此处有限等 + 喂狗)。
 *    - BSD socket + `SO_RCVTIMEO`, 防止阻塞 recv 触发 IWDG(~2s)复位。
 *
 *  数据格式: 代理返回 "key=value" 行, 数组逗号分隔, 模型记录 `;` 分隔、`:`
 *  分字段。这里全部手工逐行解析, 无需 JSON 库。
 ******************************************************************************
 */

#include "ai_dash_api.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "lwip/errno.h"
#include "iwdg.h"
#include "cmsis_os.h"
#include "uart_dbg.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* LwIP 全局网络接口(定义在 Src/lwip.c) */
extern struct netif gnetif;

#define AIDASH_RESP_MAX    4096   /* 最大可解析响应长度(扁平文本远小于此) */
#define AIDASH_WAIT_IP_MS  15000  /* 等 DHCP 拿到地址上限 */
#define AIDASH_SOCK_TM_MS  2000   /* socket 收发超时 */
#define AIDASH_CONNECT_TM_MS 5000 /* 连接超时上限 */

/* 从协议头之后(空行后)的正文开始解析 */
static const char *body_start(const char *resp)
{
  const char *p = strstr(resp, "\r\n\r\n");
  if (p != NULL)
  {
    return p + 4;
  }
  /* 兼容只有 LF 的响应 */
  p = strstr(resp, "\n\n");
  if (p != NULL)
  {
    return p + 2;
  }
  return resp;
}

/* 解析一个逗号分隔的数字序列 */
static void parse_series(const char *val, uint16_t *dst, uint16_t maxlen, uint16_t *outlen)
{
  uint16_t n = 0;
  const char *p = val;
  while (n < maxlen)
  {
    char *end = NULL;
    long v = strtol(p, &end, 10);
    if (end == p)
    {
      break;   /* 没有更多数字 */
    }
    dst[n++] = (uint16_t)(v < 0 ? 0 : v);
    if (*end != ',')
    {
      break;
    }
    p = end + 1;
  }
  *outlen = n;
}

int ai_dash_poll(ai_dash_data_t *out)
{
  char resp[AIDASH_RESP_MAX];
  int total = 0;

  /* ---- 1. 等 DHCP 拿到地址 ---- */
  {
    uint32_t t0 = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - t0) < AIDASH_WAIT_IP_MS)
    {
      HAL_IWDG_Refresh(&hiwdg);
      if (gnetif.ip_addr.addr != 0)
      {
        break;
      }
      osDelay(200);
    }
    if (gnetif.ip_addr.addr == 0)
    {
      dbg_printf("[dash] no IP (DHCP) yet\r\n");
      return -2;
    }
  }

  /* ---- 2. 建 socket 连代理 ---- */
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    dbg_printf("[dash] socket fail\r\n");
    return -3;
  }

  struct timeval tv;
  tv.tv_sec  = AIDASH_SOCK_TM_MS / 1000;
  tv.tv_usec = (AIDASH_SOCK_TM_MS % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(tv));
#ifdef SO_SNDTIMEO
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv, sizeof(tv));
#endif

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(AIDASH_PROXY_PORT);
  addr.sin_addr.s_addr = inet_addr(AIDASH_PROXY_IP);

  /* ---- 非阻塞 connect + select 限时: 目标不可达时快速失败, 且全程喂狗 ---- */
  {
    int fl = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, fl | O_NONBLOCK);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0 &&
        errno != EINPROGRESS && errno != EWOULDBLOCK)
    {
      dbg_printf("[dash] connect fail to %s:%d (errno=%d)\r\n", AIDASH_PROXY_IP, AIDASH_PROXY_PORT, errno);
      close(sock);
      return -4;
    }
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    struct timeval sel;
    sel.tv_sec  = 3;
    sel.tv_usec = 0;
    int sr;
    for (;;)
    {
      HAL_IWDG_Refresh(&hiwdg);      /* select 等待期间喂狗 */
      sr = select(sock + 1, NULL, &wfds, NULL, &sel);
      if (sr > 0)
      {
        break;                        /* 可写 = 连接完成(成功或失败, 需查 SO_ERROR) */
      }
      if (sr == 0)
      {
        dbg_printf("[dash] connect timeout to %s:%d\r\n", AIDASH_PROXY_IP, AIDASH_PROXY_PORT);
        close(sock);
        return -4;                    /* 超时 */
      }
      if (errno == EINTR)
      {
        continue;
      }
      dbg_printf("[dash] select error %d\r\n", errno);
      close(sock);
      return -4;
    }
    int soerr = 0;
    socklen_t slot = sizeof(soerr);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&soerr, &slot);
    if (soerr != 0)
    {
      dbg_printf("[dash] connect fail err=%d\r\n", soerr);
      close(sock);
      return -4;
    }
    /* 恢复阻塞, 便于后续 send/recv 语义 */
    fcntl(sock, F_SETFL, fl);
  }

  /* ---- 3. 发送请求 ---- */
  const char *req = "GET /api/dashboard HTTP/1.0\r\nHost: ds-dash\r\n\r\n";
  HAL_IWDG_Refresh(&hiwdg);
  if (send(sock, req, strlen(req), 0) < 0)
  {
    dbg_printf("[dash] send fail\r\n");
    close(sock);
    return -5;
  }

  /* ---- 4. 收响应(至连接关闭或超时) ---- */
  {
    char buf[512];
    int n;
    while (total < (int)sizeof(resp) - 1)
    {
      HAL_IWDG_Refresh(&hiwdg);
      n = recv(sock, buf, sizeof(buf) - 1, 0);
      if (n <= 0)
      {
        break;   /* 0 = 连接关闭(HTTP/1.0); <0 = 超时错误 */
      }
      memcpy(resp + total, buf, (size_t)n);
      total += n;
      resp[total] = '\0';
    }
  }
  close(sock);
  resp[total] = '\0';

  if (total == 0)
  {
    dbg_printf("[dash] empty response\r\n");
    return -6;
  }
  dbg_printf("[dash] got %d bytes\r\n", total);

  /* ---- 5. 逐行解析 ---- */
  const char *p = body_start(resp);
  while (*p != '\0')
  {
    /* 取一行(到 \n) */
    char line[256];
    int i = 0;
    while (*p != '\0' && *p != '\n' && i < (int)sizeof(line) - 1)
    {
      line[i++] = *p++;
    }
    if (*p == '\n')
    {
      p++;
    }
    line[i] = '\0';

    /* 去掉行尾 \r */
    if (i > 0 && line[i - 1] == '\r')
    {
      line[i - 1] = '\0';
    }

    char *eq = strchr(line, '=');
    if (eq == NULL)
    {
      continue;
    }
    *eq = '\0';
    const char *key = line;
    const char *val = eq + 1;

    if      (strcmp(key, "OK") == 0)      { out->ok = (uint8_t)atoi(val); }
    else if (strcmp(key, "BAL_CNY") == 0)      { strncpy(out->bal_cny, val, sizeof(out->bal_cny) - 1); }
    else if (strcmp(key, "BAL_USD") == 0)      { strncpy(out->bal_usd, val, sizeof(out->bal_usd) - 1); }
    else if (strcmp(key, "AVAIL") == 0)        { out->avail = (uint8_t)atoi(val); }
    else if (strcmp(key, "TODAY_COST_CNY") == 0) { strncpy(out->today_cost_cny, val, sizeof(out->today_cost_cny) - 1); }
    else if (strcmp(key, "TODAY_COST_USD") == 0) { strncpy(out->today_cost_usd, val, sizeof(out->today_cost_usd) - 1); }
    else if (strcmp(key, "MONTH_COST_CNY") == 0) { strncpy(out->month_cost_cny, val, sizeof(out->month_cost_cny) - 1); }
    else if (strcmp(key, "REQ_TODAY") == 0)  { out->req_today = (uint32_t)strtoul(val, NULL, 10); }
    else if (strcmp(key, "TOK_TODAY") == 0)  { out->tok_today = (uint32_t)strtoul(val, NULL, 10); }
    else if (strcmp(key, "CACHE_HIT") == 0)  { out->cache_hit = (float)atof(val); }
    else if (strcmp(key, "PEAK") == 0)       { out->peak = (uint8_t)atoi(val); }
    else if (strcmp(key, "UPDATE") == 0)     { strncpy(out->update, val, sizeof(out->update) - 1); }
    else if (strcmp(key, "REQ_SERIES") == 0)   { parse_series(val, out->req_series, AIDASH_SERIES_MAX, &out->series_len); }
    else if (strcmp(key, "IN_SERIES") == 0)    { parse_series(val, out->in_series, AIDASH_SERIES_MAX, &out->series_len); }
    else if (strcmp(key, "OUT_SERIES") == 0)   { parse_series(val, out->out_series, AIDASH_SERIES_MAX, &out->series_len); }
    else if (strcmp(key, "CACHE_SERIES") == 0) { parse_series(val, out->cache_series, AIDASH_SERIES_MAX, &out->series_len); }
    else if (strcmp(key, "MODELS") == 0)
    {
      out->model_cnt = 0;
      const char *rec = val;
      while (*rec != '\0' && out->model_cnt < AIDASH_MODEL_MAX)
      {
        const char *rec_end = strchr(rec, ';');
        if (rec_end == NULL)
        {
          rec_end = rec + strlen(rec);
        }
        /* 依次拆 "name:req:tok:out:cache:cost" */
        char fields[AIDASH_MODEL_FIELDS][64];
        uint8_t f = 0;
        const char *fp = rec;
        while (f < AIDASH_MODEL_FIELDS && fp < rec_end)
        {
          const char *colon = strchr(fp, ':');
          uint16_t len = (colon != NULL && colon < rec_end) ? (uint16_t)(colon - fp) : (uint16_t)(rec_end - fp);
          if (len >= sizeof(fields[f])) { len = sizeof(fields[f]) - 1; }
          memcpy(fields[f], fp, len);
          fields[f][len] = '\0';
          f++;
          if (colon == NULL || colon >= rec_end) { break; }
          fp = colon + 1;
        }
        if (f >= 5)
        {
          uint8_t idx = out->model_cnt;
          snprintf(out->model_name[idx], sizeof(out->model_name[idx]), "%.*s",
                   (int)sizeof(out->model_name[idx]) - 1, fields[0]);
          out->model_req[idx]  = (uint32_t)strtoul(fields[1], NULL, 10);
          out->model_tok[idx]  = (uint32_t)strtoul(fields[2], NULL, 10);
          out->model_out[idx]  = (uint32_t)strtoul(fields[3], NULL, 10);
          out->model_cache[idx] = (float)atof(fields[4]);
          snprintf(out->model_cost[idx], sizeof(out->model_cost[idx]), "%.*s",
                   (int)sizeof(out->model_cost[idx]) - 1, fields[5]);
          out->model_cnt++;
        }

        if (*rec_end == '\0') { break; }
        rec = rec_end + 1;
      }
    }
  }

  dbg_printf("[dash] parsed bal=%s req=%lu tok=%lu cache=%.1f%% models=%u\r\n",
             out->bal_cny, (unsigned long)out->req_today, (unsigned long)out->tok_today,
             (double)out->cache_hit, (unsigned)out->model_cnt);
  return 0;
}
