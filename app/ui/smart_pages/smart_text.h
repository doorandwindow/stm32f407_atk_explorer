/**
 ******************************************************************************
 * @file    smart_text.h
 * @brief   small_smart 移植 - UI 文案总表 (UTF-8)
 * @note    本文件是中文字库生成的字符集真源: 新增页面文案先加到这里,
 *          再重新生成字库 (见 docs/debug-loop.md 字库章节 / gen_fonts 命令)。
 *          数字/ASCII/内置图标(LV_SYMBOL_*)由 lv_font_conv 的 -r 0x20-0x7E
 *          与 0xF100-0xF15F 区间保证, 不需要写在这里。
 ******************************************************************************
 */
#ifndef SMART_TEXT_H
#define SMART_TEXT_H

/* ---- 环境总览 (主页, 仿源 Screen_Main1) ---- */
#define TXT_MAIN_TITLE        "环境总览"
#define TXT_TEMP_IN           "室内温度"
#define TXT_TEMP_OUT          "室外温度"
#define TXT_HUMI              "湿度"
#define TXT_CO2               "二氧化碳"
#define TXT_PA                "负压"
#define TXT_TEMP_AVR          "平均温度"
#define TXT_TAR_TEMP          "目标温度"
#define TXT_AGE_DAYS          "日龄"
#define TXT_WATER             "水表"
#define TXT_UNIT_C            "°C"
#define TXT_UNIT_HUMI         "%RH"
#define TXT_UNIT_CO2          "ppm"
#define TXT_UNIT_PA           "Pa"
#define TXT_UNIT_DAY          "天"

/* ---- 设备状态 (图标网格) ---- */
#define TXT_DEV_FAN           "风机"
#define TXT_DEV_WC            "湿帘"
#define TXT_DEV_VFD           "变频"
#define TXT_DEV_HEAT          "加热"
#define TXT_DEV_SPRAY         "喷雾"
#define TXT_DEV_WD            "小窗"
#define TXT_DEV_LIGHT         "照明"
#define TXT_STATE_ON          "运行"
#define TXT_STATE_OFF         "停止"

/* ---- 报警 ---- */
#define TXT_ALARM_TITLE       "报警列表"
#define TXT_ALARM_NONE        "无报警"
#define TXT_ALARM_HIGH_TEMP   "高温报警"
#define TXT_ALARM_LOW_TEMP    "低温报警"
#define TXT_ALARM_SENS_FAULT  "传感器故障"
#define TXT_ALARM_TIME        "发生时刻"
#define TXT_ALARM_BANNER      "设备报警!"

/* ---- 设置/导航 ---- */
#define TXT_SETUP_TITLE       "系统设置"
#define TXT_NAV_BACK          "返回"
#define TXT_NAV_HOME          "主页"
#define TXT_TAR_TEMP_SET      "目标温度设置"
#define TXT_ABOUT_TITLE       "关于"
#define TXT_ABOUT_CHIP        "主控芯片"
#define TXT_ABOUT_VER         "固件版本"
#define TXT_VER               "V1.0.0-smart"

#endif /* SMART_TEXT_H */
