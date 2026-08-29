# STM32F407 AI 全流程开发实验工程

> **实验性质声明**：本工程不是产品，而是一场测试——  
> 验证 AI 能否独立完成 STM32 嵌入式开发的完整闭环。

## 这是什么

一句话：**用多家 AI，把 STM32 开发从头到尾全部自动化。**

传统嵌入式开发链路长、环节多：需求分析、方案设计、代码编写、调试排错、测试验证，  
每一环都依赖工程师的经验。本工程的目标是**测试这条链路能否全部交给 AI**——  
从功能分析到功能验证，AI 全程主导，人工只做方向把关与最终确认。

同时，本工程接入**多家 AI 工具**进行交叉测试，对比各家在嵌入式开发场景下的真实能力，  
评估"AI 驱动嵌入式开发"这件事目前到底走到哪一步了。

## 实验目标：AI 全流程闭环

| 阶段     | 内容                  | 执行者 |
| ------ | ------------------- | --- |
| ① 功能分析 | 需求拆解、外设选型、可行性评估     | AI  |
| ② 功能确定 | 方案设计、引脚分配、接口定义      | AI  |
| ③ 功能开发 | CubeMX 配置、驱动编写、业务代码 | AI  |
| ④ 功能测试 | 编译、烧录、用例执行、问题定位     | AI  |
| ⑤ 功能验证 | 数据校验、性能评估、缺陷修复      | AI  |

**终极目标：全流程（分析 → 确定 → 开发 → 测试 → 验证）由 AI 独立完成，  
人工仅做审核与拍板，让"AI 写 STM32"从辅助工具进化为完整工作流。**

## 硬件平台

| 项   | 规格                                   |
| --- | ------------------------------------ |
| 主控  | STM32F407ZGT6（Cortex-M4F @ 168MHz）   |
| 开发板 | 正点原子探索者 V2.2                         |
| 传感器 | MPU6050 六轴 IMU                       |
| 显示  | TFTLCD 480×800 竖屏（NT35510, FSMC 8080 并口） |
| 触摸  | GT9147 电容触摸（模拟 I2C，实测 IC 为 GT917S，寄存器兼容） |
| 中间件 | FreeRTOS + LwIP（以太网）+ LVGL 8.3.11   |

## 工具链

| 环节         | 工具                       |
| ---------- | ------------------------ |
| 引脚/时钟/外设配置 | STM32CubeMX              |
| 构建系统       | CMake + Ninja            |
| 编译器        | arm-none-eabi-gcc 13.3.1 |
| AI 助手      | 多家 AI 交叉测试（见下表）          |

## 参与测试的 AI

> 持续更新，记录每家 AI 参与的环节与实测表现。

| AI 产品 | 参与环节 | 实测表现 | 备注 |
| ----- | ---- | ---- | -- |
| （待记录） |      |      |    |

## 工程结构

```
├── CMakeLists.txt / CMakePresets.json   # 构建入口
├── CubeMX_Config.ioc                    # CubeMX 硬件配置
├── config/                              # 配置文件集中管理（lv_conf.h / FreeRTOSConfig.h 等）
├── linker/                              # 链接脚本
├── startup/                             # 启动文件
├── bsp/                                 # BSP 层：板级驱动
│   ├── boards/alientek_explorer_v2.2/  # 板型配置
│   └── components/                      # 驱动组件（lcd/nt35510, touch/gt9147, uart/uart_dbg）
├── app/                                 # 应用层：模块化组织
│   ├── core/                            # 应用核心（main.c）
│   ├── tasks/                           # FreeRTOS 任务（freertos.c）
│   ├── ui/                              # 用户界面（预留）
│   └── ports/                           # 移植层（LVGL 接口）
├── scripts/                             # 自动化脚本（构建、烧录、串口监控、内存分析）
├── Inc/ Src/                            # CubeMX 生成代码（由 CubeMX 管理，勿手工改）
├── Drivers/                             # HAL / CMSIS
├── Middlewares/                         # FreeRTOS / LwIP / LVGL
├── docs/                                # 文档（architecture.md / debug-loop.md / 改进路线图）
└── cmake/                               # 工具链配置与源文件清单
```

> 详细架构说明见 [`docs/architecture.md`](docs/architecture.md)
> 改进路线图见 [`docs/structure-improvement-roadmap.md`](docs/structure-improvement-roadmap.md)

## 构建

```bash
# 配置
cmake --preset Debug

# 编译
cmake --build build/Debug

# 或使用一键脚本（推荐）
./scripts/build_and_flash.sh        # 编译 + 内存分析 + 烧录（可选）
```

产物：`build/Debug/CubeMX_Config.elf`，可用 ST-Link 烧录。

## 快速上手

```bash
# 1. 一键构建并烧录
./scripts/build_and_flash.sh

# 2. 监控串口输出
./scripts/serial_monitor.sh

# 详细脚本说明见 scripts/README.md
```

## 当前进度

- [x] 工程骨架：CubeMX + CMake/Ninja 构建链路打通
- [x] CubeMX 外设初始化：以太网（RMII）、FreeRTOS、LwIP、FSMC、SPI1/2、USART1/3、RTC、IWDG、SDIO、USB OTG、DAC、TIM
- [x] TFTLCD 驱动 `Src/lcd.c`：NT35510 480×800 竖屏，FSMC Bank4（NE4）8080 并口，背光 PB15
- [x] GT9147 触摸驱动 `Src/gt9147.c`：模拟 I2C（SCL=PB0/SDA=PF11/RST=PC13/INT=PB1），地址 0x14
      - **实测 IC 为 GT917S**（0x8140 读回 "917S"，寄存器兼容；官方例程 strcmp("9147") 在此批屏幕上同样匹配不到）
      - 串口已验证按下/抬起事件流与坐标连续性；滑动到进度条区域坐标停更已修复（`lv_bar` 事件未冒泡）
- [x] LVGL 8.3.11 集成：源码 + `lv_conf.h` + 显示/输入 port（`lv_port_disp.c` / `lv_port_indev.c`）
      - LVGL 内存池 128KB 与显示缓冲约 192KB（双缓冲，每个 480×100）置于外部 SRAM IS62WV51216（FSMC NE3，CubeMX 已配置）
      - 测试任务 `lvglTestTask`（8KB 栈）周期调度 `lv_task_handler`，界面含：进度条动画 / 按钮点击计数 / 触摸坐标实时显示 / 秒计数
      - **编译通过**（345 编译单元，FLASH 467KB / RAM 107KB @ -O0 Debug）
- [x] 业务功能：任务 `keyLedTask`（按键控制 LED）—— **已上板验证**（2026-08-29 与调光任务对换 LED 后）
      - KEY0(PE4, 低有效/内部上拉) 按下翻转 LED1(PF10, 低电平点亮)；20ms 消抖 + 松开检测防重复触发
      - 每次有效按下串口打印 `[key] KEY0 press -> LED1 ON/OFF`（实测严格交替，无误触发）
      - 引脚定义在 `bsp/boards/alientek_explorer_v2.2/board.h`（KEY0/KEY1 + LED 电平宏）
- [x] 业务功能：任务 `keyBrightTask`（长按调节 LED 亮度）—— **已上板验证**
      - KEY1(PE3, 低有效/内部上拉) 长按(≥400ms) 调节 LED0(PF9) 亮度；0%→100%→0% 往返，20ms/级
      - 亮度用 TIM14_CH1 硬件 PWM（PF9 是板上唯一硬件 PWM 引脚；1kHz / 1000 级 / 无中断）。早期版本亮度落在
        PF10（无硬件 PWM）故用 TIM13 软 PWM，LED 对换后已废弃
      - 串口打印 `[bright] KEY1 hold -> start ramping` / `duty NN%` / `release`；按住不足 400ms 为短按无动作
      - 模块在 `bsp/components/led/led_pwm.{c,h}`；对换后两任务占用 PF9(PWM)/PF10(GPIO)，互不干扰
- [ ] 上板验证（进行中）：**已修复「每 ~2.86s 复位循环」**——根因是 defaultTask 栈仅 512B 却同步跑
      `MX_LWIP_Init()`（实测调用链 ~300B），栈溢出踩坏调度器结构 → INVPC HardFault → IWDG 复位。
      栈扩至 2KB，二分双向验证（512B 必死 / 2KB 稳定）。现串口心跳正常；屏幕显示内容与触摸坐标
      方向仍需人工目视确认。完整证据链见 `debug-loop.md`
- [ ] 业务功能开发（待开始）：传感器驱动、应用逻辑等

## 声明

本工程为 AI 能力测试实验，代码由 AI 生成并经过人工验证，不代表生产级质量，  
请勿直接用于商业产品。
