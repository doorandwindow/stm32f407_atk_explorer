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

/**
  * @brief LVGL 触摸读取回调
  */
static void indev_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    static GT_Point_t pt;
    static uint8_t pressed = 0;
    static uint8_t error_reported = 0;
    GT9147_ScanResult_t result;

    result = GT9147_Scan(&pt);
    if (result == GT9147_SCAN_PRESSED)
    {
        if (!pressed)
        {
            dbg_printf("[dbg] TP pressed x=%u y=%u\r\n", pt.x, pt.y);
        }
        pressed = 1;
        error_reported = 0;
        /* 边缘越界截断: 按到玻璃边缘时 GT9147 原始值会略超 480x800, 越界点会判定
           为"没按到任何对象"导致事件丢失 */
        if (pt.x >= LCD_W) pt.x = LCD_W - 1;
        if (pt.y >= LCD_H) pt.y = LCD_H - 1;
        data->point.x = pt.x;
        data->point.y = pt.y;
    }
    else if (result == GT9147_SCAN_RELEASED)
    {
        if (pressed)
        {
            dbg_printf("[dbg] TP released\r\n");
        }
        pressed = 0;
        error_reported = 0;
    }
    else if (result == GT9147_SCAN_ERROR && !error_reported)
    {
        dbg_printf("[dbg] TP I2C error\r\n");
        error_reported = 1;
    }

    /* Keep the last point and state when the controller has no new packet or
       a transient I2C error; neither condition is a release event. */
    data->point.x = pt.x;
    data->point.y = pt.y;
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
