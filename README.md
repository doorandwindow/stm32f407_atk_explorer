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

### small_smart 移植 (2026-08-30 启动, Route B: LVGL 重绘到本板 NT35510)

来源工程: `C:\Users\syclx\Desktop\env\exmple\small_smart`(RT-Thread + 大彩串口屏养殖舍环控器)。
移植决策: 只保留 defaultTask, 旧实验任务全部退役; 界面用 LVGL 在本板重绘(横屏 800x480),
不外接串口屏; 4G 通话/短信链路(AIR724+at_client)不移植; 控制外设桩化(板上无继电器)。

- [x] **P1 清场**: 删 keyLed/keyBright/dashboard/lvglTest 四任务 + demos/dashboard_screen/
      ai_dash_api/led_pwm; monitorTask 新增逐任务栈水位行。堆占用 35.9→7.4KB
- [x] **P2 拔除 LwIP+ETH**: 构建层外科手术移除(ioc 保留原配置, 恢复路径见 cmake/stm32cubemx 注释);
      defaultTask 纯喂狗化 1KB 栈。主 SRAM 静态 109.6→45.9KB(剩 ~80KB)。
      顺带修复 rtstats 时钟回卷窗口倒退竞态([mon] cpu 荒谬值根除, 见 debug-loop.md 坑 10)
- [x] **P3 任务骨架** (`app/smart/`): smart_data 数据模型(0.1 定点 + Flag_* 节拍 + 设备状态 +
      11 项报警表, 沿源工程全局结构体模式) + smart_tick(1s 节拍, 替代 3 个 rt_timer) +
      smart_periodic(10ms 控制状态机骨架, 45s 开机保护/风机轮启框架保留, 硬件桩) +
      smart_alarm(5s 巡检: 高温/低温/传感器故障) + smart_sim(桩数据源)
- [x] **P4 界面地基**: NT35510 扫描寄存器切横屏 800x480(零软件旋转开销) + 触摸轴变换;
      中文字库 SimHei 75 字子集 16/24px(~85KB Flash, lv_font_conv 生成, 图标走 montserrat_20 回退);
      smart_ui 任务(8KB, 5ms 循环); 页面管理器(create/update 模式替代源工程页面协程)
- [x] **P5 页面**: 环境总览(默认页, 数据卡+设备状态格+报警横幅) / Setup 菜单 / 报警列表 /
      目标温度设置(±0.5°C 步进) / 关于; 全部文案落在既有字库内
- [x] **P6 收尾**: 30 分钟串口老化验收通过(2026-08-30, `build/aging_30min.log`)——全程零复位/
      零 [HOOK], CPU 稳态 5.1%, 堆 used 恒 16.7KB, LVGL 对象池 used 恒 16.5KB(删建切页无泄漏),
      各任务栈水位稳定; 文档三份同步完成
- [ ] 后续(按需): 真实传感器采集层替换 smart_sim、外部 SRAM 双缓冲、设置持久化(内部 Flash)

### 历史实验 (2026-08-27~30, P1 清场后已退役: keyLed/keyBright/dashboard/DeepSeek 仪表盘/demo 界面)

- [x] 工程骨架: CubeMX + CMake/Ninja 构建链路打通
- [x] TFTLCD 驱动: NT35510(现横屏 800x480, 原竖屏 480x800), FSMC Bank4 8080 并口, 背光 PB15
- [x] GT9147 触摸驱动: 模拟 I2C, **实测 IC 为 GT917S**(寄存器兼容, ID 校验双接受)
- [x] LVGL 8.3.11 集成: 显示/输入 port + 对象池外部 SRAM 128KB + CCM 渲染缓冲
- [x] monitorTask 诊断: 三层 RAM 水印 + 逐任务栈水位 + CPU 占用率(每 10s, `[mon]` 四行)
- [x] IWDG 看门狗: ~2s 超时, 任何 >2s 阻塞必须喂狗; 栈溢出/malloc 失败钩子全开

## 声明

本工程为 AI 能力测试实验，代码由 AI 生成并经过人工验证，不代表生产级质量，  
请勿直接用于商业产品。
