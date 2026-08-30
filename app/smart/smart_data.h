/**
 ******************************************************************************
 * @file    smart_data.h
 * @brief   small_smart 移植 - 核心数据模型 (Route B, P3 骨架)
 * @note    沿用源工程(little_spirit_code)的"全局结构体 + 标志位"模式:
 *          - 单写多读: 每个字段只有一个生产者任务, 消费者(UI)只读;
 *          - 0.1 定点整数: 温度 0.1°C / 湿度 0.1%RH / 负压 0.1Pa, 全程无浮点;
 *          - Flag_* 置 1 由消费者清 0 (源 APP_Timer.c 约定);
 *          - 探索者板上无继电器/传感器, 设备状态由 smart_sim 桩驱动,
 *            换 20 路继电器板时补 bsp 层, 状态机不动。
 ******************************************************************************
 */
#ifndef SMART_DATA_H
#define SMART_DATA_H

#include <stdint.h>

/* ---- 设备数量上限 (沿源工程 include.h, 空转设备保留位) ---- */
#define SMART_MAX_FAN    12u   /* 定速风机 */
#define SMART_MAX_WC      3u   /* 湿帘 */
#define SMART_MAX_VFD     2u   /* 变频风机 */
#define SMART_MAX_HEAT    3u   /* 定频加热 */
#define SMART_MAX_SPRAY   1u   /* 喷雾 */
#define SMART_MAX_WD      5u   /* 小窗/导流板 */
#define SMART_MAX_LIGHT   2u   /* 照明 */

/* ---- 节拍标志位: 置 1, 消费者取走后清 0 (源 APP_Timer.c 模式) ---- */
extern volatile uint8_t Flag_1s;
extern volatile uint8_t Flag_5s;
extern volatile uint8_t Flag_10s;
extern volatile uint8_t Flag_30s;
extern volatile uint8_t Flag_1m;
extern volatile uint8_t Flag_1h;
extern volatile uint8_t Flag_Fresh;    /* UI 整体刷新节拍 (10s) */

extern volatile uint32_t Timer_SecNow; /* 开机以来的秒数 (软时钟) */
extern volatile uint32_t Timer_MinNow; /* 开机以来的分钟数 */

/* ---- 实时环境数据 (源 APP_Sensor.h Data_Info 子集, 水表无硬件保留字段) ---- */
typedef struct
{
  int16_t  temp[3];     /* 室内温度 1~3, 0.1°C, -400~+850 有效 */
  int16_t  temp_out;    /* 室外温度, 0.1°C */
  int16_t  t_avr;       /* 平均温度 (启用且无故障的室内温度均值), 0.1°C */
  uint16_t humi;        /* 湿度, 0.1%RH */
  uint16_t co2;         /* CO2 浓度, ppm */
  uint16_t pa;          /* 负压, 0.1Pa */
  uint32_t water_val;   /* 水表读数 (无硬件, 恒 0) */
} smart_data_t;

extern volatile smart_data_t g_smart_data;

/* 传感器故障标志 (源 Error_Code 子集): 1=故障 */
typedef struct
{
  uint8_t temp[3];
  uint8_t temp_out;
  uint8_t humi;
  uint8_t co2;
  uint8_t pa;
} smart_error_t;

extern volatile smart_error_t g_smart_err;

/* ---- 目标温度与日龄 ---- */
extern volatile int16_t  g_tar_temp;   /* 目标温度, 0.1°C */
extern volatile uint16_t g_age_days;   /* 饲养日龄, 天 */

/* ---- 设备运行状态 (UI 图标数据源; 桩驱动, 真机接 DO 反馈) ----
 * state: 0=停, 1=运行; vfd_speed 0~1000 对应 0~100.0%; wd_pos 0~100 开度% */
typedef struct
{
  uint8_t  fan[SMART_MAX_FAN];
  uint8_t  wc[SMART_MAX_WC];
  uint8_t  vfd[SMART_MAX_VFD];
  uint16_t vfd_speed[SMART_MAX_VFD];
  uint8_t  heat[SMART_MAX_HEAT];
  uint8_t  spray[SMART_MAX_SPRAY];
  uint8_t  wd_pos[SMART_MAX_WD];
  uint8_t  light[SMART_MAX_LIGHT];
} smart_state_t;

extern volatile smart_state_t g_smart_state;

/* ---- 报警 (源 APP_Alarm.h 精简: 巡检由 smart_alarm 任务做) ----
 * 编号沿用源工程 __AlarmCnt=11 约定的前 3 项, UI 按编号映射文案 */
#define SMART_ALARM_CNT 11u

#define SMART_ALM_HIGH_TEMP   0u   /* 高温报警 */
#define SMART_ALM_LOW_TEMP    1u   /* 低温报警 */
#define SMART_ALM_SENS_FAULT  2u   /* 传感器故障 */

typedef struct
{
  uint8_t  active;   /* 1=报警中 */
  uint32_t sec;      /* 报警发生时刻 (Timer_SecNow) */
} smart_alarm_t;

extern volatile uint8_t        g_alarm_all;                  /* 任一报警中 =1 */
extern volatile smart_alarm_t  g_alarm_list[SMART_ALARM_CNT];

/* ---- 初始化: 清零全部共享数据并置初值, 在任务创建前调用一次 ---- */
void smart_data_init(void);

#endif /* SMART_DATA_H */
