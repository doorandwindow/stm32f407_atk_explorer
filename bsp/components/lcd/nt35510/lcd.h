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

/* LCD 尺寸 (2026-08-30 P4.0 横屏化 800x480, 对齐源工程大彩屏横版布局):
   LVGL 逻辑坐标系为横屏, 物理扫描方向由 LCD_SCAN_MODE 切换, 触摸变换见
   lv_port_indev.c 的 TP_LANDSCAPE_MAP。若上板发现画面/触摸方向不对,
   按 lcd.c 中 LCD_SCAN_MODE 注释表换值(一行), 或换用另一组触摸映射。 */
#define LCD_W   800
#define LCD_H   480

/* NT35510 0x3600 (Address Mode) 扫描方向值:
   0x00 = 竖屏 480x800 (L2R, U2D, 旧配置)
   0x60 = 横屏 800x480 候选A (90° 旋转, 默认)
   0xA0 = 横屏 800x480 候选B (反向 90°, 候选A 画面倒 180° 时换它, 触摸映射同时换) */
#define LCD_SCAN_MODE   0x60

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

/* ---- DMA 加速传输 ---- */
void    LCD_FillRect_DMA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                         const uint16_t *buf);  /* DMA 加速版本 */
uint8_t LCD_DMA_IsBusy(void);                   /* 检查 DMA 是否忙 */
uint8_t LCD_DMA_Poll(void);                     /* 非阻塞检查 DMA */
uint32_t LCD_DMA_GetFallbackCount(void);        /* DMA 错误/超时回退次数 */
void    LCD_DMA_Wait(void);                     /* 等待 DMA 完成 */

#endif /* __LCD_H */
