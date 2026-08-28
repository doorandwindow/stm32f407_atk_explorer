/**
  ******************************************************************************
  * @file    uart_dbg.c
  * @brief   串口调试输出（USART1 115200, 轮询发送, 不依赖 printf 重定向）
  ******************************************************************************
  */
#include "uart_dbg.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

void dbg_printf(const char *fmt, ...)
{
    char buf[160];
    va_list args;
    uint16_t len;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    len = (uint16_t)strlen(buf);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
}
