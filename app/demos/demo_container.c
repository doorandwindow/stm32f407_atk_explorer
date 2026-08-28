/**
  ******************************************************************************
  * @file    demo_container.c
  * @brief   LVGL Demo - Tab 3: 容器与布局控件展示
  ******************************************************************************
  */
#include "demo_main.h"
#include "uart_dbg.h"

/* ---- 事件回调 ---- */

/** List 按钮点击事件 */
static void list_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 1); /* List button's label is child 1 */
    const char *txt = lv_label_get_text(label);
    dbg_printf("[demo] List item clicked: %s\r\n", txt);
}

/* ---- Tab 内容创建 ---- */

/**
  * @brief  创建 Tab 3 内容：容器与布局控件
  * @param  parent: Tab 容器对象
  */
void demo_container_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 15, 0);

    /* ---- 标题 ---- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Containers & Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    /* ---- 1. Panel (普通容器) ---- */
    lv_obj_t *sec1 = lv_obj_create(parent);
    lv_obj_set_size(sec1, 440, 100);
    lv_obj_set_style_bg_color(sec1, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec1, 2, 0);
    lv_obj_set_style_border_color(sec1, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_pad_all(sec1, 10, 0);

    lv_obj_t *panel_lbl = lv_label_create(sec1);
    lv_label_set_text(panel_lbl, "Panel: A styled container\nwith border and padding");
    lv_obj_set_style_text_color(panel_lbl, lv_color_hex(0xE0E7FF), 0);
    lv_obj_align(panel_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    /* ---- 2. List ---- */
    lv_obj_t *sec2 = lv_obj_create(parent);
    lv_obj_set_size(sec2, 440, 200);
    lv_obj_set_style_bg_color(sec2, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec2, 0, 0);
    lv_obj_set_style_pad_all(sec2, 10, 0);

    lv_obj_t *list_header = lv_label_create(sec2);
    lv_label_set_text(list_header, "List:");
    lv_obj_set_style_text_color(list_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(list_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *list = lv_list_create(sec2);
    lv_obj_set_size(list, 400, 160);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_row(list, 2, 0); /* 减少行间距 */

    lv_obj_t *btn;
    btn = lv_list_add_btn(list, LV_SYMBOL_FILE, "Document 1");
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, "Folder");
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_IMAGE, "Photo.jpg");
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* ---- 3. Table ---- */
    lv_obj_t *sec3 = lv_obj_create(parent);
    lv_obj_set_size(sec3, 440, 200);
    lv_obj_set_style_bg_color(sec3, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec3, 0, 0);
    lv_obj_set_style_pad_all(sec3, 10, 0);

    lv_obj_t *table_header = lv_label_create(sec3);
    lv_label_set_text(table_header, "Table:");
    lv_obj_set_style_text_color(table_header, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(table_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *table = lv_table_create(sec3);
    lv_obj_set_size(table, 400, 160);
    lv_obj_align(table, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_table_set_col_cnt(table, 3);
    lv_table_set_row_cnt(table, 3);
    lv_table_set_col_width(table, 0, 80);
    lv_table_set_col_width(table, 1, 150);
    lv_table_set_col_width(table, 2, 80);

    /* 表头 */
    lv_table_set_cell_value(table, 0, 0, "ID");
    lv_table_set_cell_value(table, 0, 1, "Name");
    lv_table_set_cell_value(table, 0, 2, "Age");
    /* 数据行 */
    lv_table_set_cell_value(table, 1, 0, "001");
    lv_table_set_cell_value(table, 1, 1, "Alice");
    lv_table_set_cell_value(table, 1, 2, "25");
    lv_table_set_cell_value(table, 2, 0, "002");
    lv_table_set_cell_value(table, 2, 1, "Bob");
    lv_table_set_cell_value(table, 2, 2, "30");

    /* ---- 4. LED 指示灯 ---- */
    lv_obj_t *sec4 = lv_obj_create(parent);
    lv_obj_set_size(sec4, 440, 100);
    lv_obj_set_style_bg_color(sec4, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sec4, 0, 0);
    lv_obj_set_style_pad_all(sec4, 10, 0);
    lv_obj_set_flex_flow(sec4, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec4, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *led_header = lv_label_create(sec4);
    lv_label_set_text(led_header, "LED:");
    lv_obj_set_style_text_color(led_header, lv_color_hex(0x94A3B8), 0);

    lv_obj_t *led1 = lv_led_create(sec4);
    lv_obj_set_size(led1, 30, 30);
    lv_led_set_color(led1, lv_color_hex(0x10B981));
    lv_led_on(led1);

    lv_obj_t *led2 = lv_led_create(sec4);
    lv_obj_set_size(led2, 30, 30);
    lv_led_set_color(led2, lv_color_hex(0xF59E0B));
    lv_led_off(led2);

    lv_obj_t *led3 = lv_led_create(sec4);
    lv_obj_set_size(led3, 30, 30);
    lv_led_set_color(led3, lv_color_hex(0xEF4444));
    lv_led_off(led3);

    dbg_printf("[demo] Container widgets created\r\n");
}
