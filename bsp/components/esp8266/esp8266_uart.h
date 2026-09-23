/**
 ******************************************************************************
 * @file    esp8266_uart.h
 * @brief   ESP8266 串口驱动层 (N1) - USART3 中断收 + 环形缓冲 + 行分帧
 * @note    - USART3 由 CubeMX 生成 (huart3 @115200 8N1, PB10=TX/PB11=RX),
 *            本层只补 NVIC 使能 + 单字节中断接收;
 *          - 环形缓冲单生产者(ISR)/单消费者(任务), 无锁;
 *          - 行分帧在任务侧 read_line 内做, 半行状态跨调用保持;
 *          - 故意不用 DMA: 免改 ioc, 规避 CubeMX 重生成回退 (network-roadmap N1)。
 ******************************************************************************
 */
#ifndef __ESP8266_UART_H
#define __ESP8266_UART_H

#include <stdint.h>
#include <stddef.h>

#define ESP_LINE_MAX  512u   /* 单行缓冲上限: AT 响应行远小于此 */

/* 初始化: 使能 USART3 NVIC + 挂首字节中断接收 (MX_USART3_UART_Init 已由 main 调用) */
void esp8266_uart_init(void);

/* 阻塞发送 n 字节 (内部互斥, AT 问答半双工) */
void esp8266_uart_write(const uint8_t *data, size_t len);

/* 取一行 (已去 \r\n, buf 末尾补 0):
 * 返回 0=取到一行; 1=timeout_ms 内无完整行 (半行保留, 下次继续组) */
int esp8266_uart_read_line(char *buf, uint16_t max, uint32_t timeout_ms);

/* 诊断: 环满丢弃的字节数 (供电不稳/过速时会涨) */
uint32_t esp8266_uart_rx_dropped(void);

#endif /* __ESP8266_UART_H */
