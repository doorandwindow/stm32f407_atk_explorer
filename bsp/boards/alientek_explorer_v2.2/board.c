/**
 ******************************************************************************
 * @file    board.c
 * @brief   正点原子探索者 V2.2 开发板板级支持包实现
 ******************************************************************************
 */

#include "board.h"

/**
 * @brief  板级初始化（预留，主要初始化由 CubeMX 完成）
 * @note   当前版本中，GPIO/时钟/外设初始化由 CubeMX 生成的代码完成
 *         此函数可用于板级特定的额外初始化
 */
void Board_Init(void)
{
    /* 板级特定初始化（如需要） */
}

/**
 * @brief  点亮 LED
 * @param  led: LED 编号（0 或 1）
 */
void Board_LED_On(uint8_t led)
{
    if (led == 0) {
        HAL_GPIO_WritePin(BOARD_LED0_GPIO, BOARD_LED0_PIN, GPIO_PIN_SET);
    } else if (led == 1) {
        HAL_GPIO_WritePin(BOARD_LED1_GPIO, BOARD_LED1_PIN, GPIO_PIN_SET);
    }
}

/**
 * @brief  熄灭 LED
 * @param  led: LED 编号（0 或 1）
 */
void Board_LED_Off(uint8_t led)
{
    if (led == 0) {
        HAL_GPIO_WritePin(BOARD_LED0_GPIO, BOARD_LED0_PIN, GPIO_PIN_RESET);
    } else if (led == 1) {
        HAL_GPIO_WritePin(BOARD_LED1_GPIO, BOARD_LED1_PIN, GPIO_PIN_RESET);
    }
}

/**
 * @brief  翻转 LED 状态
 * @param  led: LED 编号（0 或 1）
 */
void Board_LED_Toggle(uint8_t led)
{
    if (led == 0) {
        HAL_GPIO_TogglePin(BOARD_LED0_GPIO, BOARD_LED0_PIN);
    } else if (led == 1) {
        HAL_GPIO_TogglePin(BOARD_LED1_GPIO, BOARD_LED1_PIN);
    }
}
