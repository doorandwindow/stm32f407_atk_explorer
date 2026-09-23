/**
 ******************************************************************************
 * @file    esp_task.c
 * @brief   ESP8266 链路任务 (N2 精简先行版) - WiFi/TCP 连接状态机 + 5s 心跳
 * @note    - 状态机: BOOT -> PROBE(AT判活) -> MODE -> WIFI -> TCP -> ONLINE
 *            任一层失败自动回退重连; TCP 连败 x3 回退 WiFi 层
 *          - 心跳: 5s 周期, "HB,<seq>,<uptime_s>,<heap>\r\n" 到 TCP_HOST:TCP_PORT
 *          - CIPSEND 三道保险 (2026-09-23 凑数透传事故教训):
 *            1) '>' 提示符后才发 payload; 2) 发送字节数与声明严格一致;
 *            3) 等 SEND OK, 失败即 CIPCLOSE 重置链路回 TCP 层
 *          - URC 常扫: "CLOSED"/"WIFI DISCONNECT" 触发对应层回退
 *          - IWDG 由 defaultTask 独立喂, 本任务阻塞无狗风险
 ******************************************************************************
 */
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "uart_dbg.h"
#include "esp8266_uart.h"
#include <string.h>
#include <stdio.h>

/* ---- 网络参数 (同学提供, 2026-09-23) ---- */
#define WIFI_SSID   "Redmi_43C3"
#define WIFI_PASS   "l1234567890"
#define TCP_HOST    "115.120.239.161"
#define TCP_PORT    "25082"   /* 2026-09-23 17:11 同学通知: 端口由 21583 更换 */

/* ---- 心跳参数 ---- */
#define BEAT_PERIOD_MS  5000u   /* 心跳周期: 5s */
#define BEAT_FAIL_MAX   3u      /* 连续失败 N 次重置链路 (本版单次失败即重连) */
#define TCP_RETRY_MAX   3u      /* TCP 连败 N 次回退 WiFi 层 */

/* 网络状态: 0=离线 1=WiFi已连 2=TCP在线 (UI/monitor 只读) */
volatile uint8_t g_net_status = 0;

osThreadId_t espTaskHandle;
const osThreadAttr_t espTask_attributes = {
  .name = "espTask",
  .stack_size = 4096,   /* line[512]x2 + snprintf + 状态机调用链 */
  .priority = (osPriority_t) osPriorityBelowNormal,
};

typedef enum {
  ST_BOOT = 0,   /* 等模组上电 ready */
  ST_PROBE,      /* AT 判活 (x3 重试) */
  ST_MODE,       /* CWMODE=1 */
  ST_WIFI,       /* CWJAP 连热点 (先 CIFSR 探测已在线) */
  ST_TCP,        /* CIPSTART 连服务器 */
  ST_ONLINE,     /* 5s 心跳 + URC 常扫 */
} esp_state_t;

/* ---- AT 底语 ---- */

/* 发一条 AT 命令 (自动补 \r\n) */
static void at_send(const char *cmd)
{
  esp8266_uart_write((const uint8_t *)cmd, strlen(cmd));
  esp8266_uart_write((const uint8_t *)"\r\n", 2u);
}

/* 调试回显开关: 排查期打开 (模组每行响应都上串口), 稳定后置 0 静默 */
#define ESP_VERBOSE  1

/* 等待响应: 行含 ok[] 之一 -> 返回下标(>=0); 行含 err[] 之一 -> -2; 超时 -> -1.
 * 未知行 (回显/其他 URC/上电垃圾) 忽略. */
static int at_wait(uint32_t timeout_ms,
                   const char *const *ok, int n_ok,
                   const char *const *err, int n_err)
{
  char line[ESP_LINE_MAX];
  uint32_t start = osKernelGetTickCount();

  for (;;)
  {
    if (esp8266_uart_read_line(line, sizeof(line), 20u) == 0)
    {
#if ESP_VERBOSE
      dbg_printf("[esp] << %s\r\n", line);
#endif
      for (int i = 0; i < n_ok; i++)
        if (strstr(line, ok[i]) != NULL)  return i;
      for (int i = 0; i < n_err; i++)
        if (strstr(line, err[i]) != NULL)  return -2;
      /* 未知行: 忽略 */
    }
    if ((osKernelGetTickCount() - start) >= timeout_ms)
      return -1;
  }
}

/* ---- TCP 心跳: CIPSEND -> '>' -> payload(严格 n 字节) -> SEND OK ---- */
static int tcp_beat(uint32_t seq)
{
  char tx[96];
  char cmd[32];

  uint32_t up   = osKernelGetTickCount() / 1000u;
  uint32_t heap = (uint32_t)xPortGetFreeHeapSize();
  int n = snprintf(tx, sizeof(tx), "HB,%lu,%lu,%lu\r\n",
                   (unsigned long)seq, (unsigned long)up, (unsigned long)heap);
  if (n <= 0 || n >= (int)sizeof(tx))  return -1;

  /* 1. 声明发送长度 */
  snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", n);
  at_send(cmd);

  static const char *const ok_prompt[] = {">"};
  static const char *const err_link[]  = {"ERROR", "link is not valid", "CLOSED"};
  int r = at_wait(4000u, ok_prompt, 1, err_link, 3);   /* 4s: 老固件 '>' 可能延迟 */
  if (r == -2)
    return -1;   /* 链路已死 (ERROR/CLOSED), 直接重连 */

  /* 2. 塞 payload (字节数与声明严格一致): '>' 迟到也照发 ——
   *    v1.2 老固件 '>' 实测可延迟 4s+, 但 CIPSEND 已受理 (OK 回过),
   *    payload 缓冲在模组 UART RX, '>' 出来后立即消费发送
   *    (实测: flush 后 90ms 出 '>' -> Recv -> SEND OK, 数据不丢) */
  if (r != 0)
    dbg_printf("[esp] beat#%lu '>' late (r=%d), send buffered\r\n", (unsigned long)seq, r);
  esp8266_uart_write((const uint8_t *)tx, (size_t)n);

  /* 3. 等 SEND OK —— 这才是心跳成功标准 (6s: 模组处理慢留裕量) */
  static const char *const ok_send[] = {"SEND OK"};
  static const char *const err_any[] = {"SEND FAIL", "ERROR", "CLOSED", "WIFI DISCONNECT"};
  r = at_wait(6000u, ok_send, 1, err_any, 4);
  if (r != 0)
  {
    dbg_printf("[esp] beat#%lu no SEND OK (r=%d)\r\n", (unsigned long)seq, r);
    return -1;
  }
  return 0;
}

/* ---- 任务主体: 状态机 ---- */
void StartEspTask(void *argument)
{
  (void)argument;
  esp_state_t st = ST_BOOT;
  uint32_t beat_seq = 0, beat_ok = 0, tcp_fail = 0;
  uint32_t last_beat = 0;

  dbg_printf("[esp] espTask started, heartbeat every %us -> %s:%s\r\n",
             BEAT_PERIOD_MS / 1000u, TCP_HOST, TCP_PORT);

  for (;;)
  {
    switch (st)
    {
    case ST_BOOT:   /* ESP-01S 上电 ready ~1s, 留裕量 */
      osDelay(2000);
      st = ST_PROBE;
      break;

    case ST_PROBE:  /* AT 判活, x3 */
    {
      static const char *const ok1[] = {"OK"};
      int r = -1;
      for (int i = 0; i < 3 && r != 0; i++)
      {
        at_send("AT");
        r = at_wait(1500u, ok1, 1, NULL, 0);
        if (r != 0)  osDelay(1000);
      }
      if (r == 0)
      {
        at_send("ATE0");                       /* 关回显, 信道干净 */
        (void)at_wait(1000u, ok1, 1, NULL, 0);
        dbg_printf("[esp] module alive (AT v1.2, echo off)\r\n");
        st = ST_MODE;
      }
      else
      {
        dbg_printf("[esp] module dead (AT x3), retry in 5s\r\n");
        osDelay(5000);
      }
      break;
    }

    case ST_MODE:
    {
      static const char *const ok1[] = {"OK"};
      at_send("AT+CWMODE=1");
      int r = at_wait(2000u, ok1, 1, NULL, 0);
      if (r == 0)
      {
        dbg_printf("[esp] CWMODE=1 (STA)\r\n");
        tcp_fail = 0;
        st = ST_WIFI;
      }
      else
      {
        dbg_printf("[esp] CWMODE fail (r=%d), retry\r\n", r);
        osDelay(2000);
      }
      break;
    }

    case ST_WIFI:
    {
      /* 先探测是否已在网: TCP 层回退时 WiFi 大概率还活着, 别暴力重连 */
      static const char *const ok_ip[] = {"+CIFSR:STAIP"};
      static const char *const ok_got[] = {"GOT IP"};
      static const char *const er_jap[] = {"FAIL"};

      at_send("AT+CIFSR");
      if (at_wait(2000u, ok_ip, 1, NULL, 0) == 0)
      {
        dbg_printf("[esp] WiFi already up, keep it\r\n");
        g_net_status = 1;
        tcp_fail = 0;
        st = ST_TCP;
        break;
      }

      g_net_status = 0;
      dbg_printf("[esp] connecting WiFi \"%s\" ...\r\n", WIFI_SSID);
      char cmd[96];
      snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
      at_send(cmd);
      int r = at_wait(20000u, ok_got, 1, er_jap, 1);
      if (r == 0)
      {
        dbg_printf("[esp] WiFi connected\r\n");
        g_net_status = 1;
        tcp_fail = 0;
        st = ST_TCP;
      }
      else
      {
        dbg_printf("[esp] WiFi fail (r=%d), retry in 5s\r\n", r);
        osDelay(5000);
      }
      break;
    }

    case ST_TCP:
    {
      static const char *const ok1[] = {"CONNECT", "ALREADY"};
      static const char *const er1[] = {"ERROR", "CLOSED", "NO ACK"};
      dbg_printf("[esp] connecting TCP %s:%s ...\r\n", TCP_HOST, TCP_PORT);
      char cmd[80];
      snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%s", TCP_HOST, TCP_PORT);
      at_send(cmd);
      int r = at_wait(10000u, ok1, 2, er1, 3);
      if (r >= 0)
      {
        dbg_printf("[esp] TCP connected, heartbeat every %us\r\n", BEAT_PERIOD_MS / 1000u);
        g_net_status = 2;
        /* 首包延迟 1s: 老固件 v1.2 的 CIPSEND 阻塞在内部连接收尾上,
         * 连接后立即 CIPSEND 会让 '>' 迟迟不出 (实测), 给模组留处理时间 */
        last_beat = osKernelGetTickCount() - BEAT_PERIOD_MS + 1000u;
        st = ST_ONLINE;
      }
      else
      {
        tcp_fail++;
        dbg_printf("[esp] TCP fail #%lu (r=%d)\r\n", (unsigned long)tcp_fail, r);
        at_send("AT+CIPCLOSE");                 /* 清场: 防模组残留半连接干扰下次 CIPSTART */
        (void)at_wait(2000u, NULL, 0, NULL, 0);
        if (tcp_fail >= TCP_RETRY_MAX * 2u)
        {
          /* 连败 6 次 (两轮): 模组内部状态疑似脏透 (老固件长跑后已知),
           * AT+RST 硬重启模组, 从判活重新走全流程 */
          dbg_printf("[esp] TCP x%lu, AT+RST reset module\r\n", (unsigned long)tcp_fail);
          at_send("AT+RST");
          osDelay(5000);                        /* 模组重启 + boot */
          (void)at_wait(3000u, NULL, 0, NULL, 0);   /* 吃掉 ready 前垃圾行 */
          g_net_status = 0;
          tcp_fail = 0;
          st = ST_PROBE;
        }
        else if (tcp_fail >= TCP_RETRY_MAX)
        {
          dbg_printf("[esp] TCP x%lu, back to WiFi layer\r\n", (unsigned long)tcp_fail);
          st = ST_WIFI;
        }
        else
        {
          osDelay(3000);
        }
      }
      break;
    }

    case ST_ONLINE:
    {
      /* URC 常扫 (50ms 快扫): 服务器断开/WiFi 掉线立即感知 */
      static const char *const er_urc[] = {"CLOSED", "WIFI DISCONNECT"};
      int r = at_wait(50u, NULL, 0, er_urc, 2);
      if (r == -2)
      {
        dbg_printf("[esp] URC: link lost, back to TCP layer\r\n");
        g_net_status = 1;
        st = ST_TCP;
        break;
      }

      uint32_t now = osKernelGetTickCount();
      if ((now - last_beat) >= BEAT_PERIOD_MS)
      {
        last_beat = now;
        beat_seq++;
        if (tcp_beat(beat_seq) == 0)
        {
          beat_ok++;
          if (beat_ok % 6u == 0)   /* 每 30s 报一次活性, 平时静默 */
          {
            dbg_printf("[esp] beat summary: %lu sent, %lu fail-reset, dropped=%lu\r\n",
                       (unsigned long)beat_ok, 0u, (unsigned long)esp8266_uart_rx_dropped());
          }
        }
        else
        {
          /* 心跳失败 -> AT+RST 硬重启模组, 不用 CIPCLOSE:
           * 模组可能滞留 "等 payload" 态 (供电欠压丢字节凑不满声明数),
           * 此态下一切命令文本 (含 CIPCLOSE) 都会被当心跳数据吃掉发上 TCP
           * (2026-09-23 17:20 服务器收到 AT+CIPCLOSE/AT 垃圾的根因).
           * RST 是唯一可靠的态清除手段, 重启后从判活重走全流程 (~15s). */
          dbg_printf("[esp] beat#%lu FAIL -> AT+RST (clear stuck payload-wait)\r\n",
                     (unsigned long)beat_seq);
          at_send("AT+RST");
          osDelay(5000);                          /* 模组重启 + boot */
          (void)at_wait(3000u, NULL, 0, NULL, 0); /* 吃掉 ready 前垃圾行 */
          g_net_status = 0;
          tcp_fail = 0;
          st = ST_PROBE;
        }
      }
      osDelay(50);
      break;
    }

    default:
      st = ST_PROBE;
      break;
    }
  }
}
