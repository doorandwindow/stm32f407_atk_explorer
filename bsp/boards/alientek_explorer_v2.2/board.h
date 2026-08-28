/**
 ******************************************************************************
 * @file    board.h
 * @brief   正点原子探索者 V2.2 开发板硬件配置
 * @board   STM32F407ZGT6 开发板
 * @vendor  正点原子 (Alientek)
 * @version V2.2
 ******************************************************************************
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ============================================================================
   板级基本信息
   ========================================================================= */
#define BOARD_NAME              "Alientek Explorer V2.2"
#define BOARD_MCU               "STM32F407ZGT6"
#define BOARD_VENDOR            "Alientek"

/* ============================================================================
   LCD 配置
   ========================================================================= */
#define BOARD_LCD_CONTROLLER    "NT35510"
#define BOARD_LCD_WIDTH         480
#define BOARD_LCD_HEIGHT        800
#define BOARD_LCD_ORIENTATION   0       /* 0: 竖屏, 1: 横屏 */

/* LCD FSMC 配置 */
#define BOARD_LCD_FSMC_BANK     FSMC_NORSRAM_BANK4
#define BOARD_LCD_FSMC_NE       4       /* NE4 */
#define BOARD_LCD_FSMC_RS_PIN   6       /* A6 作为 RS(寄存器选择) */

/* LCD 背光引脚 */
#define BOARD_LCD_BL_GPIO       GPIOB
#define BOARD_LCD_BL_PIN        GPIO_PIN_15

/* ============================================================================
   触摸屏配置
   ========================================================================= */
#define BOARD_TOUCH_CONTROLLER  "GT9147"   /* 实测 IC 为 GT917S，寄存器兼容 */
#define BOARD_TOUCH_I2C_ADDR    0x14       /* 7 位地址 */

/* 模拟 I2C 引脚 */
#define BOARD_TOUCH_SCL_GPIO    GPIOB
#define BOARD_TOUCH_SCL_PIN     GPIO_PIN_0

#define BOARD_TOUCH_SDA_GPIO    GPIOF
#define BOARD_TOUCH_SDA_PIN     GPIO_PIN_11

#define BOARD_TOUCH_RST_GPIO    GPIOC
#define BOARD_TOUCH_RST_PIN     GPIO_PIN_13

#define BOARD_TOUCH_INT_GPIO    GPIOB
#define BOARD_TOUCH_INT_PIN     GPIO_PIN_1

/* ============================================================================
   调试串口配置
   ========================================================================= */
#define BOARD_DEBUG_UART        USART1
#define BOARD_DEBUG_BAUDRATE    115200

/* ============================================================================
   LED 配置
   ========================================================================= */
#define BOARD_LED0_GPIO         GPIOF
#define BOARD_LED0_PIN          GPIO_PIN_9

#define BOARD_LED1_GPIO         GPIOF
#define BOARD_LED1_PIN          GPIO_PIN_10

/* ============================================================================
   外部 SRAM 配置
   ========================================================================= */
#define BOARD_EXT_SRAM_MODEL    "IS62WV51216"
#define BOARD_EXT_SRAM_SIZE     (1024 * 1024)  /* 1MB */
#define BOARD_EXT_SRAM_BASE     0x68000000     /* FSMC Bank1 NE3 */

/* LVGL 内存布局（使用外部 SRAM） */
#define BOARD_LVGL_MEM_BASE     0x68000000     /* LVGL 内存池起始地址 */
#define BOARD_LVGL_MEM_SIZE     (128 * 1024)   /* 128KB */
#define BOARD_LVGL_BUF_BASE     0x68020000     /* LVGL 显示缓冲起始地址 */
#define BOARD_LVGL_BUF_SIZE     (192 * 1024)   /* 双缓冲，每个 480×100×2 字节 */

/* ============================================================================
   看门狗配置
   ========================================================================= */
#define BOARD_IWDG_TIMEOUT_MS   2050           /* IWDG 超时时间约 2.05s */

/* ============================================================================
   板级初始化函数
   ========================================================================= */
void Board_Init(void);
void Board_LED_On(uint8_t led);
void Board_LED_Off(uint8_t led);
void Board_LED_Toggle(uint8_t led);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
