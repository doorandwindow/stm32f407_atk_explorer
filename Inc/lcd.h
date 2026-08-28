/**
  ******************************************************************************
  * @file    lcd.h
  * @brief   TFTLCD 驱动头文件（正点原子探索者 V2.2 / NT35510 480x800 竖屏）
  *
  * 接口: FSMC Bank1 NE4(PG12) + A6(PF12, RS) + D[15:0], 16bit 8080 并口
  * 背光: PB15（与 SPI2_MOSI 复用, 探索者官方设计）
  * 复位: 接开发板复位脚, 无需软件控制
  ******************************************************************************
  */
#ifndef __LCD_H
#define __LCD_H

#include "main.h"

/* LCD 尺寸（竖屏） */
#define LCD_W   480
#define LCD_H   800

/* 背光引脚 */
#define LCD_BL_GPIO_PORT   GPIOB
#define LCD_BL_PIN         GPIO_PIN_15

/* LCD 片选（FSMC_NE4） */
#define LCD_CS_GPIO_PORT   GPIOG
#define LCD_CS_PIN         GPIO_PIN_12

/* ---- 基础操作（供 LVGL port 使用） ---- */
void    LCD_Init(void);
uint16_t LCD_ReadID(void);
void    LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void    LCD_WriteRAMPrepare(void);
void    LCD_WriteRAM(uint16_t color);          /* 写单个像素（须先 SetWindow） */
void    LCD_Clear(uint16_t color);
void    LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                     const uint16_t *buf);     /* 批量填充区域（LVGL flush 用） */

#endif /* __LCD_H */
