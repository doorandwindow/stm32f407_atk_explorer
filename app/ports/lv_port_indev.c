/**
  ******************************************************************************
  * @file    lv_port_indev.c
  * @brief   LVGL 输入设备移植（GT9147 电容触摸）
  ******************************************************************************
  */
#include "lvgl.h"
#include "gt9147.h"
#include "lcd.h"
#include "uart_dbg.h"

static lv_indev_drv_t indev_drv;

/* ---- 触摸健康度: 连续 I2C 错误计数 (PRESSED/RELEASED 即清零) ----
   供电欠压把 GT917S 打挂时 I2C 持续 NACK, 此计数持续上涨;
   smart_ui 主循环据此触发 GT 重初始化自愈 (2026-09-23 ESP8266 接入后新增) */
static volatile uint16_t s_tp_err_run = 0;

uint16_t lv_port_indev_tp_err_run(void)  { return s_tp_err_run; }
void     lv_port_indev_tp_err_reset(void){ s_tp_err_run = 0; }

/* ---- P4.0 横屏触摸映射 ----
   GT9147 原生输出为面板物理坐标: x∈[0,479](480 轴), y∈[0,799](800 轴)。
   横屏后 LVGL 的 x 轴即面板 800 轴, y 轴即 480 轴, 需要轴交换 + 镜像。
   与 lcd.h 的 LCD_SCAN_MODE 配套:
     LCD_SCAN_MODE=0x60 -> TP_MAP_FLIP=1 (ly = 479-原生x)
     画面倒 180° 换 0xA0 -> TP_MAP_FLIP=0 (ly = 原生x, 同时 lx 改 799-原生y) */
#define TP_MAP_FLIP 1

/**
  * @brief LVGL 触摸读取回调
  */
static void indev_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    static GT_Point_t pt;
    static uint8_t pressed = 0;
    static uint8_t error_reported = 0;
    static uint16_t last_lx = 0, last_ly = 0;   /* 最近一次映射后的 LVGL 坐标 */
    GT9147_ScanResult_t result;

    result = GT9147_Scan(&pt);
    if (result == GT9147_SCAN_PRESSED)
    {
        s_tp_err_run = 0;
        if (!pressed)
        {
            dbg_printf("[dbg] TP raw x=%u y=%u\r\n", pt.x, pt.y);
        }
        pressed = 1;
        error_reported = 0;

        /* 物理坐标 -> LVGL 横屏坐标: 轴交换, x 走面板 800 轴 */
        uint16_t lx = pt.y;
        uint16_t ly = pt.x;
#if TP_MAP_FLIP
        ly = (LCD_H - 1) - ly;
#else
        lx = (LCD_W - 1) - lx;
#endif
        /* 边缘越界截断: 按到玻璃边缘时 GT9147 原始值会略超量程, 越界点会判定
           为"没按到任何对象"导致事件丢失 */
        if (lx >= LCD_W) lx = LCD_W - 1;
        if (ly >= LCD_H) ly = LCD_H - 1;
        last_lx = lx;
        last_ly = ly;
        data->point.x = lx;
        data->point.y = ly;
    }
    else if (result == GT9147_SCAN_RELEASED)
    {
        s_tp_err_run = 0;
        if (pressed)
        {
            dbg_printf("[dbg] TP released\r\n");
        }
        pressed = 0;
        error_reported = 0;
    }
    else if (result == GT9147_SCAN_ERROR)
    {
        if (s_tp_err_run < 0xFFFF) s_tp_err_run++;
        if (!error_reported)
        {
            dbg_printf("[dbg] TP I2C error\r\n");
            error_reported = 1;
        }
    }

    /* Keep the last point and state when the controller has no new packet or
       a transient I2C error; neither condition is a release event. */
    data->point.x = last_lx;
    data->point.y = last_ly;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/**
  * @brief LVGL 输入设备初始化（须在 lv_init 之后调用）
  */
void lv_port_indev_init(void)
{
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = indev_read;
    lv_indev_drv_register(&indev_drv);
}
