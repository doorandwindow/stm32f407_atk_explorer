/**
  ******************************************************************************
  * @file    demo_main.h
  * @brief   LVGL 全功能 Demo 主入口
  ******************************************************************************
  */
#ifndef DEMO_MAIN_H
#define DEMO_MAIN_H

#include "lvgl.h"

/* Demo 主界面创建（Tabview 框架） */
void demo_create(void);

/* 极简性能测试版本 */
void demo_create_simple(void);

/* 各 Tab 内容创建函数 */
void demo_basic_create(lv_obj_t *parent);
void demo_input_create(lv_obj_t *parent);
void demo_container_create(lv_obj_t *parent);
void demo_advanced_create(lv_obj_t *parent);
void demo_advanced_set_active(bool active);

#endif /* DEMO_MAIN_H */
