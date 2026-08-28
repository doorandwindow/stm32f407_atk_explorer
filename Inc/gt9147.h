/**
  ******************************************************************************
  * @file    gt9147.h
  * @brief   GT9147/GT917S 电容触摸驱动（正点原子探索者 V2.2 4.3寸屏）
  *
  * 接口: 模拟 I2C, CT_SCL=PB0 / CT_SDA=PF11 / CT_RST=PC13 / CT_INT=PB1
  * 地址: 0x14 (7bit), 写 0x28 / 读 0x29
  * [实测] 本屏 IC 为 GT917S (ID="917S"), 寄存器兼容 GT9147
  ******************************************************************************
  */
#ifndef __GT9147_H
#define __GT9147_H

#include "main.h"

/* ---- 触摸引脚（探索者官方口径） ---- */
#define GT_SCL_PORT   GPIOB
#define GT_SCL_PIN    GPIO_PIN_0
#define GT_SDA_PORT   GPIOF
#define GT_SDA_PIN    GPIO_PIN_11
#define GT_RST_PORT   GPIOC
#define GT_RST_PIN    GPIO_PIN_13
#define GT_INT_PORT   GPIOB
#define GT_INT_PIN    GPIO_PIN_1

/* GT9147 器件地址（含读写位: 写 0x28 / 读 0x29） */
#define GT_ADDR_W     0x28
#define GT_ADDR_R     0x29

/* 关键寄存器 */
#define GT_REG_CMD    0x8040   /* 控制命令: 写 2 软复位, 写 0 结束复位 */
#define GT_REG_CFG    0x8047   /* 配置寄存器组起始 */
#define GT_REG_PID    0x8140   /* 产品 ID: '9','1','4','7' */
#define GT_REG_STATUS 0x814E   /* 状态: bit7 buffer 有效, 低4位触点个数 */
#define GT_REG_POINT1 0x8150   /* 触点1坐标数据 */

/* 坐标数据结构（LVGL indev 用） */
typedef struct
{
    uint16_t x;
    uint16_t y;
} GT_Point_t;

/* GT9147_Scan() result.  No new packet is not the same as a release. */
typedef enum
{
    GT9147_SCAN_ERROR = 0,
    GT9147_SCAN_PRESSED,
    GT9147_SCAN_RELEASED,
    GT9147_SCAN_NO_DATA
} GT9147_ScanResult_t;

uint8_t  GT9147_Init(void);                          /* 返回 1=成功 0=失败 */
GT9147_ScanResult_t GT9147_Scan(GT_Point_t *point);  /* 按下/抬起/无新数据/错误 */

#endif /* __GT9147_H */
