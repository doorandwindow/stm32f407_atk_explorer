/**
  ******************************************************************************
  * @file    lv_port_disp.c
  * @brief   LVGL 显示驱动移植（探索者 TFTLCD 480x800 竖屏, NT35510）
  *
 * 渲染缓冲: 单缓冲 480x60, 位于 CCM RAM 0x10000000(64KB)。
 *       LVGL 对象/样式内存池仍在外部 SRAM(0x68000000, 128KB)，但渲染热路径
 *       逐像素读写用 CCM 快得多：CCM 是 CPU 直访、不进 FSMC，这是滑动流畅度的关键。
 *       注意: CCM 不能被 DMA 访问，所以 flush 用 CPU 顺序写(读 CCM -> 写 LCD)。
 ******************************************************************************
  */
#include "lvgl.h"
#include "lcd.h"

/* 部分刷新行数: 单缓冲 480 * 60 * 2B = 57.6KB, 放 CCM(0x10000000, 64KB)。
   渲染目标在 CCM 远快于外部 FSMC SRAM, 这是降低 lv_task_handler 耗时的关键。
   单缓冲即可(CCM 不能 DMA, 双缓冲无叠加收益)。 */
#define DISP_BUF_LINES   60
#define DISP_BUF_PIXELS  (LCD_W * DISP_BUF_LINES)

#define CCM_BUF_BASE  0x10000000UL

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static lv_color_t *buf1 = (lv_color_t *)CCM_BUF_BASE;
static uint32_t flush_count;
static uint32_t flush_pixels;

/**
  * @brief LVGL flush 回调: 将渲染缓冲区写入 LCD 指定区域
  */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    /* 缓冲在 CCM, 不能被 DMA 访问, 因此用 CPU 顺序写(读 CCM -> 写 LCD)。
       LVGL 渲染(写 CCM)已很快, 这里的 ~40ns/像素是 FSMC 写屏时间, 不再是主要瓶颈。 */
    LCD_FillRect((uint16_t)area->x1, (uint16_t)area->y1,
                 (uint16_t)area->x2, (uint16_t)area->y2,
                 (const uint16_t *)color_p);
    flush_count++;
    flush_pixels += (uint32_t)(area->x2 - area->x1 + 1) *
                    (uint32_t)(area->y2 - area->y1 + 1);
    lv_disp_flush_ready(drv);
}

void lv_port_disp_get_stats(uint32_t *count, uint32_t *pixels)
{
    if (count != NULL) *count = flush_count;
    if (pixels != NULL) *pixels = flush_pixels;
    flush_count = 0;
    flush_pixels = 0;
}

/**
  * @brief LVGL 显示驱动初始化（须在 lv_init 之后调用）
  */
void lv_port_disp_init(void)
{
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, DISP_BUF_PIXELS);  /* 单缓冲(buf2=NULL) */

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_W;
    disp_drv.ver_res = LCD_H;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
