/**
 ******************************************************************************
 * @file    net_probe_task.c
 * @brief   [临时] N0 硬件体检任务 - ESP8266 摸底: AT 活性/版本/连 WiFi/TCP 试连
 * @note    N0 体检通过后本文件整体退役 (保留探测序列可搬进 N2 引擎自检)。
 *          探测结论全走 [esp] 前缀打印到 USART1 调试口。
 *          IWDG 由 defaultTask 独立喂, 本任务长阻塞无狗风险。
 ******************************************************************************
 */
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "uart_dbg.h"
#include "esp8266_uart.h"
#include <string.h>

/* N0 体检参数 (同学提供, 2026-09-23) */
#define WIFI_SSID   "Redmi_43C3"
#define WIFI_PASS   "l1234567890"
#define TCP_HOST    "115.120.239.161"
#define TCP_PORT    "21583"

osThreadId_t probeTaskHandle;
const osThreadAttr_t probeTask_attributes = {
  .name = "probeTask",
  .stack_size = 2048,   /* line[512] + dbg_printf(vsnprintf ~160B) 调用链 */
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* 发一条 AT 命令并把窗口期内的响应行全部回显 */
static void probe_cmd(const char *cmd, uint32_t window_ms)
{
  char line[ESP_LINE_MAX];

  dbg_printf("[esp] >> %s\r\n", cmd);
  esp8266_uart_write((const uint8_t *)cmd, strlen(cmd));
  esp8266_uart_write((const uint8_t *)"\r\n", 2u);

  uint32_t start = osKernelGetTickCount();
  while ((osKernelGetTickCount() - start) < window_ms)
  {
    if (esp8266_uart_read_line(line, sizeof(line), 20u) == 0)
    {
      dbg_printf("[esp] << %s\r\n", line);
      start = osKernelGetTickCount();   /* 有新行就续窗口, 直到静默 */
    }
  }
}

/* TCP 试发: CIPSEND 长度 -> 等 '>' -> 发数据 -> 等 SEND OK */
static void probe_tcp_send(void)
{
  char line[ESP_LINE_MAX];
  const char *payload = "hello from stm32f407\r\n";   /* 23 字节 */
  const char *cs = "AT+CIPSEND=23";

  dbg_printf("[esp] >> %s\r\n", cs);
  esp8266_uart_write((const uint8_t *)cs, strlen(cs));
  esp8266_uart_write((const uint8_t *)"\r\n", 2u);

  /* 等 '>' 提示符 (窗口 2s) */
  uint32_t start = osKernelGetTickCount();
  uint8_t got_prompt = 0;
  while ((osKernelGetTickCount() - start) < 2000u)
  {
    if (esp8266_uart_read_line(line, sizeof(line), 20u) == 0)
    {
      dbg_printf("[esp] << %s\r\n", line);
      if (line[0] == '>')  { got_prompt = 1; break; }
    }
  }
  if (!got_prompt)
  {
    dbg_printf("[esp] !! no '>' prompt, CIPSEND abort\r\n");
    return;
  }

  osDelay(20);   /* 提示符后稍歇再塞数据 */
  esp8266_uart_write((const uint8_t *)payload, 23u);

  /* 等 SEND OK (窗口 3s) */
  start = osKernelGetTickCount();
  while ((osKernelGetTickCount() - start) < 3000u)
  {
    if (esp8266_uart_read_line(line, sizeof(line), 20u) == 0)
      dbg_printf("[esp] << %s\r\n", line);
  }
}

void StartNetProbeTask(void *argument)
{
  (void)argument;

  dbg_printf("[esp] probe started, wait 2s for ESP-01S boot\r\n");
  osDelay(2000);   /* ESP-01S 上电 ready ~1s, 留裕量 */

  /* 1. 判活 */
  probe_cmd("AT", 1000u);
  /* 2. 固件版本 (MQTT 能力决策依据, network-roadmap N0) */
  probe_cmd("AT+GMR", 1500u);
  /* 3. 模式确认 */
  probe_cmd("AT+CWMODE?", 1000u);
  /* 4. 扫描验证射频 (期望见 Redmi_43C3) */
  probe_cmd("AT+CWLAP", 8000u);

  /* 5. 连热点 (发射电流峰值时刻, 供电不稳会在这里现形) */
  probe_cmd("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASS "\"", 15000u);
  /* 6. 拿 IP */
  probe_cmd("AT+CIFSR", 2000u);

  /* 7. TCP 试连同学的服务器 */
  probe_cmd("AT+CIPSTART=\"TCP\",\"" TCP_HOST "\"," TCP_PORT, 15000u);

  /* 8. 试发一包 */
  probe_tcp_send();

  dbg_printf("[esp] PROBE DONE (dropped=%lu)\r\n",
             (unsigned long)esp8266_uart_rx_dropped());

  probeTaskHandle = NULL;
  vTaskDelete(NULL);   /* 体检完自删, 临时任务不留尸 */
}
