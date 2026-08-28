# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

STM32F407ZGT6 固件工程（正点原子探索者 V2.2 开发板），实验性质：测试 AI 能否独立完成嵌入式开发全流程（详见 README.md）。技术栈：CubeMX 生成 + CMake/Ninja 构建 + arm-none-eabi-gcc，中间件 FreeRTOS + LVGL 8.3.11 + LwIP。代码注释与文档使用中文。

## 常用命令

```bash
cmake --preset Debug            # 配置（首次或改 CMakeLists 后）
cmake --build build/Debug       # 编译
cmake --build build/Release     # Release 构建（-Os）
```

- 前提：`arm-none-eabi-gcc` 在 PATH 中（工具链配置见 `cmake/gcc-arm-none-eabi.cmake`）。
- 产物：`build/Debug/CubeMX_Config.elf`；链接时打印内存占用（--print-memory-usage）。
- clangd 依赖 `build/Debug/compile_commands.json`（见 `.clangd`），先 configure。
- 上板验证走「编译 → ST-Link 烧录 → 串口捕获」闭环（可用 stm32-debug-loop skill）；调试串口为 **USART1 @ 115200**，用 `dbg_printf()` 输出（轮询发送，未做 printf 重定向）。上板参数（COM18）、RAM 布局速查与故障档案见 `debug-loop.md`。
- 无测试框架；"测试" = 编译 + 上板串口日志验证。

## 架构

### CubeMX 生成代码的边界（重要）

- `CubeMX_Config.ioc` 是硬件配置的唯一真源（引脚/时钟/外设/FreeRTOS/LwIP）。
- `cmake/stm32cubemx/CMakeLists.txt` 与 `Src/`、`Inc/` 中带 `/* USER CODE BEGIN/END */` 标记的文件由 CubeMX 重新生成时覆盖。**用户代码只写在 USER CODE 区内**；写在生成区里的手工修改会在重新生成时丢失（实例：`Src/iwdg.c` 的 IWDG 预分频 /16 是手工改的，注释已标明会被还原）。
- **新增源文件/头文件路径改根目录 `CMakeLists.txt`**（`target_sources` / `target_include_directories`），不要改 `cmake/stm32cubemx/`。LVGL 源码用 `file(GLOB_RECURSE)` 收集，`lv_conf.h` 在仓库根目录（宏 `LV_CONF_INCLUDE_SIMPLE`）。

### 启动与任务流

`main.c`: 外设 MX_*_Init → `MX_FREERTOS_Init()`（`Src/freertos.c`）→ `osKernelStart()`。
注意：`MX_RTC_Init` 和 `MX_SDIO_SD_Init` 目前被注释（诊断原因，见行内注释）。

FreeRTOS 任务（CMSIS-RTOS2，堆 32KB，见 `Inc/FreeRTOSConfig.h`）：
- `defaultTask`（512B 栈）：`MX_LWIP_Init()` + 死循环喂 IWDG。
- `lvglTestTask`（8KB 栈）：`LCD_Init` → `GT9147_Init` → `lv_init` + `lv_port_disp_init` + `lv_port_indev_init` → 建测试界面 → 5ms 循环（`lv_tick_inc` / `lv_task_handler` / 喂狗）。

### 硬件资源与内存布局（跨文件约定，勿破坏）

| 资源 | 配置 |
| --- | --- |
| LCD | NT35510 480×800 竖屏，FSMC Bank1 NE4（RS=A6），16bit 8080 并口，背光 PB15；初始化序列在 `Src/lcd_nt35510_init.inc` |
| 外部 SRAM | IS62WV51216，FSMC NE3，基址 0x68000000：LVGL 内存池 128KB（0x68000000~0x68020000）+ LVGL 双缓冲 96KB（0x68020000 起，480×50×2×2），见 `Src/lv_port_disp.c`，**两段不可重叠** |
| 触摸 | GT9147，模拟 I2C（SCL=PB0 / SDA=PF11 / RST=PC13 / INT=PB1），7 位地址 0x14。**实测 IC 为 GT917S**（ID "917S"，寄存器兼容）；CT_INT 复位释放后不可推挽驱动，全程保持输入上拉 |
| IWDG | 约 2.05s 超时（/16 + 4095）。**任何可能阻塞 >2s 的循环/初始化必须喂狗** `HAL_IWDG_Refresh(&hiwdg)`，否则复位 |

## 约定

- README.md 的「当前进度」清单只记录实际完成并验证过的内容，完成新功能后同步更新。
- 文档、注释、commit message 均使用中文；commit 遵循 conventional commits（feat/fix/docs/chore...）。
