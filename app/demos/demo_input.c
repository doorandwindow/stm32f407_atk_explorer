/**
  ******************************************************************************
  * @file    demo_input.c
  * @brief   LVGL Demo - Tab 2: 输入与选择控件展示
  ******************************************************************************
  */
#include "demo_main.h"
#include "uart_dbg.h"

/* ---- 事件回调 ---- */

/** Dropdown 选择变化 */
static void dropdown_event_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t sel = lv_dropdown_get_selected(dd);
    char buf[32];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
    lv_label_set_text_fmt(label, "Selected: %s (idx=%u)", buf, sel);
    dbg_printf("[demo] Dropdown: %s\r\n", buf);
}

/** Roller 选择变化 */
static void roller_event_cb(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t sel = lv_roller_get_selected(roller);
    char buf[16];
    lv_roller_get_selected_str(roller, buf, sizeof(buf));
    lv_label_set_text_fmt(label, "Hour: %s", buf);
}

/** Textarea 文本变化 */
static void textarea_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    const char *txt = lv_textarea_get_text(ta);
    lv_label_set_text_fmt(label, "Text: \"%s\"", txt);
}

static void textarea_focus_event_cb(lv_event_t *e)
{
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED) lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    else if (code == LV_EVENT_DEFOCUSED) lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

/* ---- Tab 内容创建 ---- */

/**
  * @brief  创建 Tab 2 内容：输入与选择控件
  * @param  parent: Tab 容器对象
  */
void demo_input_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 15, 0);

    /* ---- 标题 ---- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Input & Selection");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    /* ---- 1. Dropdown ---- */
    lv_obj_t *sec1 = lv_obj_create(parent);
    lv_obj_set_size(sec1, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec1, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec1, 0, 0);
    lv_obj_set_style_pad_all(sec1, 10, 0);
    lv_obj_set_flex_flow(sec1, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *dd_header = lv_label_create(sec1);
    lv_label_set_text(dd_header, "Dropdown:");
    lv_obj_set_style_text_color(dd_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *dd_label = lv_label_create(sec1);
    lv_label_set_text(dd_label, "Selected: Apple (idx=0)");
    lv_obj_set_style_text_color(dd_label, lv_color_hex(0x10B981), 0);

    lv_obj_t *dd = lv_dropdown_create(sec1);
    lv_dropdown_set_options(dd, "Apple\nBanana\nOrange\nGrape\nMango");
    lv_obj_set_width(dd, 200);
    lv_obj_add_event_cb(dd, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, dd_label);

    /* ---- 2. Roller ---- */
    lv_obj_t *sec2 = lv_obj_create(parent);
    lv_obj_set_size(sec2, 440, 160);
    lv_obj_set_style_bg_color(sec2, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec2, 0, 0);
    lv_obj_set_style_pad_all(sec2, 10, 0);

    lv_obj_t *roller_header = lv_label_create(sec2);
    lv_label_set_text(roller_header, "Roller:");
    lv_obj_set_style_text_color(roller_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(roller_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *roller_label = lv_label_create(sec2);
    lv_label_set_text(roller_label, "Hour: 12");
    lv_obj_set_style_text_color(roller_label, lv_color_hex(0x3B82F6), 0);
    lv_obj_align(roller_label, LV_ALIGN_TOP_LEFT, 0, 30);

    lv_obj_t *roller = lv_roller_create(sec2);
    lv_roller_set_options(roller,
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"
        "12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 3);
    lv_obj_set_width(roller, 80);
    lv_obj_align(roller, LV_ALIGN_CENTER, 0, 15);
    lv_roller_set_selected(roller, 12, LV_ANIM_OFF);
    lv_obj_add_event_cb(roller, roller_event_cb, LV_EVENT_VALUE_CHANGED, roller_label);

    /* ---- 3. Textarea ---- */
    lv_obj_t *sec3 = lv_obj_create(parent);
    lv_obj_set_size(sec3, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sec3, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec3, 0, 0);
    lv_obj_set_style_pad_all(sec3, 10, 0);
    lv_obj_set_flex_flow(sec3, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *ta_header = lv_label_create(sec3);
    lv_label_set_text(ta_header, "Textarea:");
    lv_obj_set_style_text_color(ta_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *ta_label = lv_label_create(sec3);
    lv_label_set_text(ta_label, "Text: \"Hello\"");
    lv_obj_set_style_text_color(ta_label, lv_color_hex(0xF59E0B), 0);

    lv_obj_t *ta = lv_textarea_create(sec3);
    lv_obj_set_size(ta, 400, 60);
    lv_textarea_set_placeholder_text(ta, "Type here...");
    lv_textarea_set_text(ta, "Hello");
    lv_textarea_set_one_line(ta, false);
    lv_textarea_set_max_length(ta, 64);
    lv_obj_add_event_cb(ta, textarea_event_cb, LV_EVENT_VALUE_CHANGED, ta_label);

    /* ---- 4. Keyboard (简化展示：只显示静态键盘) ---- */
    lv_obj_t *sec4 = lv_obj_create(parent);
    lv_obj_set_size(sec4, 440, 240);
    lv_obj_set_style_bg_color(sec4, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec4, 0, 0);
    lv_obj_set_style_pad_all(sec4, 10, 0);

    lv_obj_t *kb_header = lv_label_create(sec4);
    lv_label_set_text(kb_header, "Keyboard (static):");
    lv_obj_set_style_text_color(kb_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(kb_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *kb = lv_keyboard_create(sec4);
    lv_obj_set_size(kb, 420, 200);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ta, textarea_focus_event_cb, LV_EVENT_ALL, kb);

    dbg_printf("[demo] Input widgets created\r\n");
}
