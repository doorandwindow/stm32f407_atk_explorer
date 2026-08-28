/**
  ******************************************************************************
  * @file    lv_port_disp.c
  * @brief   LVGL 显示驱动移植（探索者 TFTLCD 480x800 竖屏, NT35510）
  *
  * 缓冲: 双缓冲 480x50, 位于外部 SRAM IS62WV51216 (0x68020000)
  *       LVGL 内存池占用 0x68000000 ~ 0x68020000 (128KB), 二者不重叠
  ******************************************************************************
  */
#include "lvgl.h"
#include "lcd.h"

/* 部分刷新行数: 480 * 50 * 2B * 2(双缓冲) = 96KB 外部 SRAM */
#define DISP_BUF_LINES   50
#define DISP_BUF_PIXELS  (LCD_W * DISP_BUF_LINES)

#define EXT_SRAM_BUF_BASE  0x68020000UL

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static lv_color_t *buf1 = (lv_color_t *)EXT_SRAM_BUF_BASE;
static lv_color_t *buf2 = (lv_color_t *)(EXT_SRAM_BUF_BASE + DISP_BUF_PIXELS * sizeof(lv_color_t));

/**
  * @brief LVGL flush 回调: 将渲染缓冲区写入 LCD 指定区域
  */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    LCD_FillRect((uint16_t)area->x1, (uint16_t)area->y1,
                 (uint16_t)area->x2, (uint16_t)area->y2,
                 (const uint16_t *)color_p);
    lv_disp_flush_ready(drv);
}

/**
  * @brief LVGL 显示驱动初始化（须在 lv_init 之后调用）
  */
void lv_port_disp_init(void)
{
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_BUF_PIXELS);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_W;
    disp_drv.ver_res = LCD_H;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
