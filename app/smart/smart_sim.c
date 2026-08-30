/**
 ******************************************************************************
 * @file    smart_sim.c
 * @brief   small_smart 移植 - 桩数据源 (探索者板无传感器/继电器)
 * @note    以 1s 节拍生成缓慢变化的模拟环境数据并驱动设备状态翻转,
 *          让 P4/P5 的 UI 有活数据可显示; 接真 20 路继电器板时整体退役,
 *          由 APP_Sensor/WK2124 采集层替换本文件的调用点。
 ******************************************************************************
 */
#include "smart_sim.h"
#include "smart_data.h"

/* 模拟状态: 三角波相位/方向, 简单确定性, 便于肉眼核对 UI */
static int16_t  sim_dir = 1;        /* 温度漂移方向 +1/-1 */
static uint16_t sim_sec = 0;        /* 模拟秒计数 */
static uint8_t  sim_fan_idx = 0;    /* 轮流翻转的风机序号 */

/**
 * @brief  1s 调用一次: 刷新模拟环境数据与设备状态
 */
void smart_sim_update(void)
{
  smart_data_t *d = (smart_data_t *)&g_smart_data;
  smart_state_t *s = (smart_state_t *)&g_smart_state;

  sim_sec++;

  /* 室内温度: 围绕目标温度 ±3.0°C 缓慢三角波漂移 (0.01°C/s 档) */
  if (sim_sec % 10U == 0U)   /* 每 10s 动 0.1°C, 便于观察 */
  {
    d->temp[0] += sim_dir;
    if (d->temp[0] > g_tar_temp + 30)  { sim_dir = -1; }
    if (d->temp[0] < g_tar_temp - 30)  { sim_dir = 1; }
    d->temp[1] = d->temp[0] - 5;      /* 2 号传感器低 0.5°C */
    d->temp[2] = d->temp[0] + 4;      /* 3 号传感器高 0.4°C */
    d->temp_out = 180 + (int16_t)(sim_sec % 600U) / 10U;  /* 18.0~24.0°C 慢漂 */

    d->humi = 600 + (uint16_t)((sim_sec / 10U) % 150U);   /* 60.0~75.0%RH */
    d->co2  = 2200 + (uint16_t)((sim_sec / 10U) % 800U);  /* 2200~3000ppm */
    d->pa   = 120 + (uint16_t)((sim_sec / 10U) % 60U);    /* 12.0~18.0Pa */

    /* 平均温度: 简单均值 (P3 骨架不判故障; 真机沿 Get_TempAvr 逻辑) */
    d->t_avr = (int16_t)((d->temp[0] + d->temp[1] + d->temp[2]) / 3);
  }

  /* 设备状态: 每 15s 轮流翻转一台风机, 每 47s 翻转湿帘/加热/照明各一档 */
  if (sim_sec % 15U == 0U)
  {
    s->fan[sim_fan_idx] ^= 1U;
    sim_fan_idx = (uint8_t)((sim_fan_idx + 1U) % 4U);
  }
  if (sim_sec % 47U == 0U)
  {
    s->wc[0] ^= 1U;
    s->heat[0] ^= 1U;
    s->light[0] ^= 1U;
  }
  if (sim_sec % 31U == 0U)
  {
    s->vfd[0] ^= 1U;
    s->vfd_speed[0] = s->vfd[0] ? 450U : 0U;   /* 45.0% */
  }
  /* 小窗开度: 0~100% 缓慢往返 */
  {
    static int8_t wd_dir = 1;
    static uint8_t wd = 0;
    wd = (uint8_t)(wd + (uint8_t)wd_dir);
    if (wd >= 100U) { wd = 100U; wd_dir = -1; }
    if (wd == 0U)   { wd_dir = 1; }
    s->wd_pos[0] = wd;
  }
}
