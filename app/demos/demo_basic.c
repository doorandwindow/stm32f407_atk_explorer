/**
  ******************************************************************************
  * @file    demo_basic.c
  * @brief   LVGL Demo - Tab 1: 基础控件展示
  ******************************************************************************
  */
#include "demo_main.h"
#include "uart_dbg.h"

/* ---- 事件回调 ---- */

/** 按钮点击事件 */
static void btn_click_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    static uint32_t cnt = 0;
    lv_label_set_text_fmt(label, "Clicked: %lu", ++cnt);
}

/** Toggle 按钮状态变化 */
static void btn_toggle_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    bool checked = lv_obj_has_state(btn, LV_STATE_CHECKED);
    lv_label_set_text(label, checked ? "ON" : "OFF");
    dbg_printf("[demo] Toggle: %s\r\n", checked ? "ON" : "OFF");
}

/** Checkbox 状态变化 */
static void checkbox_event_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
    lv_label_set_text_fmt(label, "Checkbox: %s", checked ? "✓" : "□");
}

/** Switch 状态变化 */
static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    lv_label_set_text_fmt(label, "Switch: %s", on ? "ON" : "OFF");
}

/** Slider 值变化 */
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    int32_t val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(label, "Value: %ld", val);
}

/** Arc 值变化 */
static void arc_event_cb(lv_event_t *e)
{
    lv_obj_t *arc = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    int16_t val = lv_arc_get_value(arc);
    lv_label_set_text_fmt(label, "%d°", val);
}

/* ---- Tab 内容创建 ---- */

/**
  * @brief  创建 Tab 1 内容：基础控件
  * @param  parent: Tab 容器对象
  */
void demo_basic_create(lv_obj_t *parent)
{
    /* 使内容区域可滚动 */
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 15, 0);

    /* ---- 标题 ---- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Basic Widgets");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    /* ---- 1. Label 示例 ---- */
    lv_obj_t *sec1 = lv_obj_create(parent);
    lv_obj_set_size(sec1, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec1, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec1, 0, 0);
    lv_obj_set_style_pad_all(sec1, 10, 0);
    lv_obj_set_flex_flow(sec1, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *lbl_header = lv_label_create(sec1);
    lv_label_set_text(lbl_header, "Label:");
    lv_obj_set_style_text_color(lbl_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *lbl1 = lv_label_create(sec1);
    lv_label_set_text(lbl1, "Default text");
    lv_obj_set_style_text_color(lbl1, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *lbl2 = lv_label_create(sec1);
    lv_label_set_text(lbl2, "Colored & Bold");
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_20, 0);

    /* ---- 2. Button 示例 ---- */
    lv_obj_t *sec2 = lv_obj_create(parent);
    lv_obj_set_size(sec2, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec2, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec2, 0, 0);
    lv_obj_set_style_pad_all(sec2, 10, 0);
    lv_obj_set_flex_flow(sec2, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *btn_header = lv_label_create(sec2);
    lv_label_set_text(btn_header, "Button:");
    lv_obj_set_style_text_color(btn_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *btn1 = lv_btn_create(sec2);
    lv_obj_set_size(btn1, 200, 50);
    lv_obj_t *btn1_label = lv_label_create(btn1);
    lv_label_set_text(btn1_label, "Click Me!");
    lv_obj_center(btn1_label);
    lv_obj_add_event_cb(btn1, btn_click_event_cb, LV_EVENT_CLICKED, NULL);

    /* Toggle Button */
    lv_obj_t *toggle_label = lv_label_create(sec2);
    lv_label_set_text(toggle_label, "OFF");
    lv_obj_set_style_text_color(toggle_label, lv_color_hex(0xFBBF24), 0);

    lv_obj_t *btn_toggle = lv_btn_create(sec2);
    lv_obj_set_size(btn_toggle, 200, 50);
    lv_obj_add_flag(btn_toggle, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_t *btn_toggle_lbl = lv_label_create(btn_toggle);
    lv_label_set_text(btn_toggle_lbl, "Toggle");
    lv_obj_center(btn_toggle_lbl);
    lv_obj_add_event_cb(btn_toggle, btn_toggle_event_cb, LV_EVENT_VALUE_CHANGED, toggle_label);

    /* ---- 3. Checkbox ---- */
    lv_obj_t *sec3 = lv_obj_create(parent);
    lv_obj_set_size(sec3, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec3, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec3, 0, 0);
    lv_obj_set_style_pad_all(sec3, 10, 0);
    lv_obj_set_flex_flow(sec3, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *cb_header = lv_label_create(sec3);
    lv_label_set_text(cb_header, "Checkbox:");
    lv_obj_set_style_text_color(cb_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *cb_label = lv_label_create(sec3);
    lv_label_set_text(cb_label, "Checkbox: □");
    lv_obj_set_style_text_color(cb_label, lv_color_hex(0x10B981), 0);

    lv_obj_t *cb = lv_checkbox_create(sec3);
    lv_checkbox_set_text(cb, "Enable feature");
    lv_obj_add_event_cb(cb, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, cb_label);

    /* ---- 4. Switch ---- */
    lv_obj_t *sec4 = lv_obj_create(parent);
    lv_obj_set_size(sec4, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec4, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec4, 0, 0);
    lv_obj_set_style_pad_all(sec4, 10, 0);
    lv_obj_set_flex_flow(sec4, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *sw_header = lv_label_create(sec4);
    lv_label_set_text(sw_header, "Switch:");
    lv_obj_set_style_text_color(sw_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *sw_label = lv_label_create(sec4);
    lv_label_set_text(sw_label, "Switch: OFF");
    lv_obj_set_style_text_color(sw_label, lv_color_hex(0xF59E0B), 0);

    lv_obj_t *sw = lv_switch_create(sec4);
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, sw_label);

    /* ---- 5. Slider ---- */
    lv_obj_t *sec5 = lv_obj_create(parent);
    lv_obj_set_size(sec5, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec5, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec5, 0, 0);
    lv_obj_set_style_pad_all(sec5, 10, 0);
    lv_obj_set_flex_flow(sec5, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *slider_header = lv_label_create(sec5);
    lv_label_set_text(slider_header, "Slider:");
    lv_obj_set_style_text_color(slider_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *slider_label = lv_label_create(sec5);
    lv_label_set_text(slider_label, "Value: 50");
    lv_obj_set_style_text_color(slider_label, lv_color_hex(0x8B5CF6), 0);

    lv_obj_t *slider = lv_slider_create(sec5);
    lv_obj_set_width(slider, 400);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, slider_label);

    /* ---- 6. Bar (Progress) ---- */
    lv_obj_t *sec6 = lv_obj_create(parent);
    lv_obj_set_size(sec6, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec6, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec6, 0, 0);
    lv_obj_set_style_pad_all(sec6, 10, 0);
    lv_obj_set_flex_flow(sec6, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *bar_header = lv_label_create(sec6);
    lv_label_set_text(bar_header, "Progress Bar:");
    lv_obj_set_style_text_color(bar_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *bar = lv_bar_create(sec6);
    lv_obj_set_size(bar, 400, 20);
    lv_bar_set_range(bar, 0, 100);
    /* Avoid starting a background animation on an inactive tab. */
    lv_bar_set_value(bar, 75, LV_ANIM_OFF);

    /* ---- 7. Arc (圆弧) ---- */
    lv_obj_t *sec7 = lv_obj_create(parent);
    lv_obj_set_size(sec7, 440, 180);
    lv_obj_set_style_bg_color(sec7, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec7, 0, 0);
    lv_obj_set_style_pad_all(sec7, 10, 0);

    lv_obj_t *arc_header = lv_label_create(sec7);
    lv_label_set_text(arc_header, "Arc:");
    lv_obj_set_style_text_color(arc_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(arc_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *arc = lv_arc_create(sec7);
    lv_obj_set_size(arc, 120, 120);
    lv_arc_set_range(arc, 0, 360);
    lv_arc_set_value(arc, 135);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *arc_label = lv_label_create(arc);
    lv_label_set_text(arc_label, "135°");
    lv_obj_set_style_text_color(arc_label, lv_color_hex(0xEC4899), 0);
    lv_obj_center(arc_label);
    lv_obj_add_event_cb(arc, arc_event_cb, LV_EVENT_VALUE_CHANGED, arc_label);

    dbg_printf("[demo] Basic widgets created\r\n");
}
