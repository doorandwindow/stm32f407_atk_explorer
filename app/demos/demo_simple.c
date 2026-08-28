/**
  ******************************************************************************
  * @file    demo_simple.c
  * @brief   LVGL 极简 Demo - 性能测试版本
  ******************************************************************************
  */
#include "demo_main.h"
#include "uart_dbg.h"

/**
  * @brief  创建极简测试界面（只有基础控件，无复杂效果）
  */
void demo_create_simple(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Performance Test");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* 按钮 */
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Button");
    lv_obj_center(btn_label);

    /* 滑块 */
    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_set_width(slider, 300);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 30);

    /* 状态标签 */
    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "Simple UI - No animations");
    lv_obj_set_style_text_color(status, lv_color_hex(0x888888), 0);
    lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -20);

    dbg_printf("[demo] Simple UI created\r\n");
}
