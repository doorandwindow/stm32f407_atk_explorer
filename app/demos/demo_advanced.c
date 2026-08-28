/**
  ******************************************************************************
  * @file    demo_advanced.c
  * @brief   LVGL Demo - Tab 4: 高级特性展示
  ******************************************************************************
  */
#include "demo_main.h"
#include "uart_dbg.h"

/* ---- 动画对象（需要全局保留，供主循环更新） ---- */
static lv_obj_t *spinner = NULL;
static lv_obj_t *anim_bar = NULL;
static uint8_t anim_bar_val = 0;
static bool advanced_active = false;
static uint32_t anim_bar_last_update = 0;

/* ---- 事件回调 ---- */

/** 动画按钮点击事件 */
static void anim_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    /* 创建位置动画：按钮左右移动 */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);
    lv_anim_set_values(&a, -100, 100);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_playback_time(&a, 500);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    dbg_printf("[demo] Animation started\r\n");
}

/** Message Box 按钮点击 */
static void msgbox_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    dbg_printf("[demo] MessageBox button clicked\r\n");
    lv_msgbox_close(mbox);
}

static void show_msgbox_event_cb(lv_event_t *e)
{
    static const char *btns[] = {"OK", "Cancel", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Alert", "This is a message box!", btns, false);
    lv_obj_add_event_cb(mbox, msgbox_btn_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
}

/* ---- 外部接口：供主循环调用的动画更新 ---- */

/**
  * @brief  更新 Tab 4 中的动画控件（由主循环周期调用）
  */
void demo_advanced_update(void)
{
    if (advanced_active && anim_bar != NULL)
    {
        uint32_t now = lv_tick_get();
        if ((uint32_t)(now - anim_bar_last_update) < 50U) return;
        anim_bar_last_update = now;

        anim_bar_val += 2;
        if (anim_bar_val > 100) anim_bar_val = 0;
        lv_bar_set_value(anim_bar, anim_bar_val, LV_ANIM_OFF);
    }
}

void demo_advanced_set_active(bool active)
{
    advanced_active = active;
    if (spinner != NULL) {
        if (active) lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ---- Tab 内容创建 ---- */

/**
  * @brief  创建 Tab 4 内容：高级特性
  * @param  parent: Tab 容器对象
  */
void demo_advanced_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 15, 0);

    /* ---- 标题 ---- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Advanced Features");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    /* ---- 1. Animation 动画 ---- */
    lv_obj_t *sec1 = lv_obj_create(parent);
    lv_obj_set_size(sec1, 440, 120);
    lv_obj_set_style_bg_color(sec1, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec1, 0, 0);
    lv_obj_set_style_pad_all(sec1, 10, 0);

    lv_obj_t *anim_header = lv_label_create(sec1);
    lv_label_set_text(anim_header, "Animation:");
    lv_obj_set_style_text_color(anim_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(anim_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *anim_btn = lv_btn_create(sec1);
    lv_obj_set_size(anim_btn, 150, 50);
    lv_obj_align(anim_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_t *anim_btn_label = lv_label_create(anim_btn);
    lv_label_set_text(anim_btn_label, "Animate!");
    lv_obj_center(anim_btn_label);
    lv_obj_add_event_cb(anim_btn, anim_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* ---- 2. Spinner 加载动画 ---- */
    lv_obj_t *sec2 = lv_obj_create(parent);
    lv_obj_set_size(sec2, 440, 120);
    lv_obj_set_style_bg_color(sec2, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec2, 0, 0);
    lv_obj_set_style_pad_all(sec2, 10, 0);

    lv_obj_t *spinner_header = lv_label_create(sec2);
    lv_label_set_text(spinner_header, "Spinner:");
    lv_obj_set_style_text_color(spinner_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(spinner_header, LV_ALIGN_TOP_LEFT, 0, 0);

    spinner = lv_spinner_create(sec2, 1000, 60);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 10);

    /* ---- 3. 自动循环进度条 ---- */
    lv_obj_t *sec3 = lv_obj_create(parent);
    lv_obj_set_size(sec3, 440, 80);
    lv_obj_set_style_bg_color(sec3, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec3, 0, 0);
    lv_obj_set_style_pad_all(sec3, 10, 0);
    lv_obj_set_flex_flow(sec3, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *bar_header = lv_label_create(sec3);
    lv_label_set_text(bar_header, "Auto Progress:");
    lv_obj_set_style_text_color(bar_header, lv_color_hex(0x94A3B8), 0);

    anim_bar = lv_bar_create(sec3);
    lv_obj_set_size(anim_bar, 400, 20);
    lv_bar_set_range(anim_bar, 0, 100);
    lv_bar_set_value(anim_bar, 0, LV_ANIM_OFF);

    /* ---- 4. Message Box 触发按钮 ---- */
    lv_obj_t *sec4 = lv_obj_create(parent);
    lv_obj_set_size(sec4, 440, 100);
    lv_obj_set_style_bg_color(sec4, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec4, 0, 0);
    lv_obj_set_style_pad_all(sec4, 10, 0);

    lv_obj_t *mbox_header = lv_label_create(sec4);
    lv_label_set_text(mbox_header, "Message Box:");
    lv_obj_set_style_text_color(mbox_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(mbox_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *mbox_btn = lv_btn_create(sec4);
    lv_obj_set_size(mbox_btn, 180, 50);
    lv_obj_align(mbox_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_t *mbox_btn_label = lv_label_create(mbox_btn);
    lv_label_set_text(mbox_btn_label, "Show MsgBox");
    lv_obj_center(mbox_btn_label);
    lv_obj_add_event_cb(mbox_btn, show_msgbox_event_cb, LV_EVENT_CLICKED, NULL);

    /* ---- 5. 滚动容器演示 ---- */
    lv_obj_t *sec5 = lv_obj_create(parent);
    lv_obj_set_size(sec5, 440, 200);
    lv_obj_set_style_bg_color(sec5, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec5, 2, 0);
    lv_obj_set_style_border_color(sec5, lv_color_hex(0x8B5CF6), 0);
    lv_obj_set_style_pad_all(sec5, 10, 0);

    lv_obj_t *scroll_header = lv_label_create(sec5);
    lv_label_set_text(scroll_header, "Scroll Container:");
    lv_obj_set_style_text_color(scroll_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(scroll_header, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 可滚动内容 */
    lv_obj_t *scroll_content = lv_obj_create(sec5);
    lv_obj_set_size(scroll_content, 400, 150);
    lv_obj_align(scroll_content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(scroll_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(scroll_content, lv_color_hex(0x1E293B), 0);

    for (int i = 0; i < 6; i++)  /* 减少到 6 项 */
    {
        lv_obj_t *item = lv_label_create(scroll_content);
        lv_label_set_text_fmt(item, "Scrollable Item %d", i + 1);
        lv_obj_set_style_text_color(item, lv_color_hex(0xE0E7FF), 0);
        lv_obj_set_style_pad_all(item, 5, 0);
    }

    /* ---- 6. Canvas 绘图（简化示例） ---- */
    lv_obj_t *sec6 = lv_obj_create(parent);
    lv_obj_set_size(sec6, 440, 180);
    lv_obj_set_style_bg_color(sec6, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec6, 0, 0);
    lv_obj_set_style_pad_all(sec6, 10, 0);

    lv_obj_t *canvas_header = lv_label_create(sec6);
    lv_label_set_text(canvas_header, "Canvas Drawing:");
    lv_obj_set_style_text_color(canvas_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(canvas_header, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Canvas 缓冲区（放到外部 SRAM，避免内部 RAM 溢出） */
    #define EXT_SRAM_CANVAS_BASE  0x68080000UL  /* 位于 480x200 双缓冲之后 */
    static lv_color_t *cbuf = (lv_color_t *)EXT_SRAM_CANVAS_BASE;

    lv_obj_t *canvas = lv_canvas_create(sec6);
    lv_canvas_set_buffer(canvas, cbuf, 150, 100, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 20);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    /* 绘制一些形状 */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFF6B6B);
    lv_canvas_draw_rect(canvas, 10, 10, 50, 30, &rect_dsc);

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(0x4ECDC4);
    line_dsc.width = 3;
    lv_point_t points[] = {{70, 20}, {130, 50}, {90, 80}};
    lv_canvas_draw_line(canvas, points, 3, &line_dsc);

    dbg_printf("[demo] Advanced widgets created\r\n");
}
