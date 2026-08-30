/**
 ******************************************************************************
 * @file    smart_data.c
 * @brief   small_smart 移植 - 核心数据模型定义与初始化
 ******************************************************************************
 */
#include "smart_data.h"
#include <string.h>

/* ---- 节拍标志位 ---- */
volatile uint8_t  Flag_1s   = 0;
volatile uint8_t  Flag_5s   = 0;
volatile uint8_t  Flag_10s  = 0;
volatile uint8_t  Flag_30s  = 0;
volatile uint8_t  Flag_1m   = 0;
volatile uint8_t  Flag_1h   = 0;
volatile uint8_t  Flag_Fresh = 0;

volatile uint32_t Timer_SecNow = 0;
volatile uint32_t Timer_MinNow = 0;

/* ---- 实时环境数据 ---- */
volatile smart_data_t g_smart_data;
volatile smart_error_t g_smart_err;

volatile int16_t  g_tar_temp = 260;   /* 初值 26.0°C (源工程雏鸡目标温度量级) */
volatile uint16_t g_age_days = 1;

/* ---- 设备运行状态 ---- */
volatile smart_state_t g_smart_state;

/* ---- 报警 ---- */
volatile uint8_t       g_alarm_all = 0;
volatile smart_alarm_t g_alarm_list[SMART_ALARM_CNT];

void smart_data_init(void)
{
  memset((void *)&g_smart_data, 0, sizeof(g_smart_data));
  memset((void *)&g_smart_err, 0, sizeof(g_smart_err));
  memset((void *)&g_smart_state, 0, sizeof(g_smart_state));
  memset((void *)g_alarm_list, 0, sizeof(g_alarm_list));
  g_alarm_all = 0;
  g_tar_temp  = 260;
  g_age_days  = 1;

  /* 模拟环境从目标温度附近起步, 避免桩数据漂移数小时才进入正常区间 */
  g_smart_data.temp[0]  = 262;   /* 26.2°C */
  g_smart_data.temp[1]  = 257;
  g_smart_data.temp[2]  = 266;
  g_smart_data.t_avr    = 262;
  g_smart_data.temp_out = 205;   /* 20.5°C */
  g_smart_data.humi     = 640;   /* 64.0%RH */
  g_smart_data.co2      = 2450;
  g_smart_data.pa       = 150;   /* 15.0Pa */
}
