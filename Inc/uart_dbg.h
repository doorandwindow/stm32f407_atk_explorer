/**
  ******************************************************************************
  * @file    uart_dbg.h
  * @brief   串口调试输出（USART1 115200, 轮询发送）
  ******************************************************************************
  */
#ifndef __UART_DBG_H
#define __UART_DBG_H

#include <stdarg.h>

void dbg_printf(const char *fmt, ...);

#endif /* __UART_DBG_H */
