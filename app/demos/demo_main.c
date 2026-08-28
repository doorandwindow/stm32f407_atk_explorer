/**
  ******************************************************************************
  * @file    demo_main.c
  * @brief   LVGL 全功能 Demo 主入口 - Tabview 框架
  ******************************************************************************
  */
#include "demo_main.h"
#include "uart_dbg.h"

static lv_obj_t *tabview = NULL;

/* Tab buttons already switch without animation; this also covers swipe input. */
static void tab_scroll_begin_event_cb(lv_event_t *e)
{
    lv_anim_t *anim = (lv_anim_t *)lv_event_get_param(e);
    if (anim != NULL) lv_anim_set_time(anim, 1U);
}

/**
  * @brief  Tab 切换事件回调（用于优化：暂停非活动页动画等）
  */
static void tab_changed_event_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    uint16_t active_id = lv_tabview_get_tab_act(tv);
    demo_advanced_set_active(active_id == 3);
}

/**
  * @brief  创建 LVGL Demo 主界面（Tabview + 4 个子页面）
  */
void demo_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E293B), 0);

    /* 创建 Tabview: 顶部标签栏，4 个 Tab */
    tabview = lv_tabview_create(scr, LV_DIR_TOP, 50);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x0F172A), 0);

    /* Disable the content scroll animation, including swipe-based switching. */
    lv_obj_t *content = lv_tabview_get_content(tabview);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(content, tab_scroll_begin_event_cb,
                        LV_EVENT_SCROLL_BEGIN, NULL);

    /* 添加 4 个 Tab */
    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Basic");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "Input");
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "Layout");
    lv_obj_t *tab4 = lv_tabview_add_tab(tabview, "Advanced");

    /* 设置 Tab 内容区域样式 */
    lv_obj_set_style_bg_color(tab1, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_color(tab2, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_color(tab3, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_color(tab4, lv_color_hex(0x1E293B), 0);

    /* 关闭各 Tab 页的滚动惯性: 快速甩动不再一次性滚动大片、触发大幅整幅重绘,
       从源头减少"整屏刷屏"导致的卡顿(代价: 松手即停, 不滑行) */
    lv_obj_clear_flag(tab1, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(tab3, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(tab4, LV_OBJ_FLAG_SCROLL_MOMENTUM);

    /* 创建各 Tab 内容 */
    demo_basic_create(tab1);
    demo_input_create(tab2);
    demo_container_create(tab3);
    demo_advanced_create(tab4);
    demo_advanced_set_active(false);

    /* 监听 Tab 切换事件 */
    lv_obj_add_event_cb(tabview, tab_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    dbg_printf("[demo] Demo界面创建完成\r\n");
}
