/**
 ******************************************************************************
 * @file    esp8266_uart.c
 * @brief   ESP8266 串口驱动层 (N1) 实现
 ******************************************************************************
 */
#include "esp8266_uart.h"
#include "usart.h"
#include "cmsis_os.h"

/* ---- 环形缓冲: 单生产者(ISR)写 head, 单消费者(任务)写 tail, 无锁 ---- */
#define RX_RING_SIZE  1024u   /* 2 的幂, 取模用位与 */

static uint8_t  s_ring[RX_RING_SIZE];
static volatile uint16_t s_head = 0;   /* ISR 写 */
static volatile uint16_t s_tail = 0;   /* 任务读 */
static volatile uint32_t s_dropped = 0;
static uint8_t  s_rx_byte;             /* Receive_IT 单字节落点 */

/* ---- 行分帧状态 (任务侧, 半行跨调用保持) ---- */
static char     s_line[ESP_LINE_MAX];
static uint16_t s_line_len = 0;

/* ---- 发送互斥: AT 问答半双工 ---- */
static osMutexId_t s_tx_mutex;

void esp8266_uart_init(void)
{
  /* NVIC: 优先级 6 (>5, 不与 FreeRTOS 临界区冲突); ISR 内不调 FreeRTOS API */
  HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);

  s_tx_mutex = osMutexNew(NULL);

  /* 挂首字节接收, 之后在 RxCpltCallback 内自续 */
  HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1);
}

void esp8266_uart_write(const uint8_t *data, size_t len)
{
  if (s_tx_mutex) osMutexAcquire(s_tx_mutex, osWaitForever);
  /* 115200 阻塞发: 86us/字节, AT 命令 <100B ≈ <12ms, 可接受 */
  HAL_UART_Transmit(&huart3, (uint8_t *)data, (uint16_t)len, 1000u);
  if (s_tx_mutex) osMutexRelease(s_tx_mutex);
}

int esp8266_uart_read_line(char *buf, uint16_t max, uint32_t timeout_ms)
{
  uint32_t start = osKernelGetTickCount();

  for (;;)
  {
    /* 出环组行 */
    while (s_tail != s_head)
    {
      char c = (char)s_ring[s_tail];
      s_tail = (uint16_t)((s_tail + 1u) & (RX_RING_SIZE - 1u));

      if (c == '\n')
      {
        uint16_t n = s_line_len;
        s_line_len = 0;
        if (n == 0u)  continue;               /* 空行跳过 */
        if (s_line[n - 1u] == '\r')  n--;     /* 去 \r */
        s_line[n] = '\0';
        uint16_t copy = (n < max - 1u) ? n : (uint16_t)(max - 1u);
        for (uint16_t i = 0; i < copy; i++)  buf[i] = s_line[i];
        buf[copy] = '\0';
        return 0;                             /* 完整一行 */
      }

      if (s_line_len < ESP_LINE_MAX - 1u)
        s_line[s_line_len++] = c;
      else
        s_line_len = 0;                       /* 行超长: 丢弃防串台 */
    }

    if ((osKernelGetTickCount() - start) >= timeout_ms)
      return 1;                                /* 超时, 半行保留 */

    osDelay(2);
  }
}

uint32_t esp8266_uart_rx_dropped(void)
{
  return s_dropped;
}

/* ---- ISR 层 ---- */

void USART3_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart3);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    uint16_t next = (uint16_t)((s_head + 1u) & (RX_RING_SIZE - 1u));
    if (next != s_tail)
    {
      s_ring[s_head] = s_rx_byte;
      s_head = next;
    }
    else
    {
      s_dropped++;   /* 环满: 上层没及时取, 计数暴露 */
    }
    HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1);   /* 自续 */
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    /* ORE/FE/NE: HAL 已锁错误状态, 直接重挂接收恢复 */
    HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1);
  }
}
