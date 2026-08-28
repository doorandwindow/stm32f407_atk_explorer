# 项目结构改进路线图

## 当前状态评估（2026-08-28）

### ✅ 已达到的专业标准

1. **分层架构**：BSP / 应用层 / 配置层分离
2. **配置集中化**：config/ 目录统一管理
3. **文档完善**：架构说明 + 迁移记录 + 调试指南
4. **构建系统**：CMake + 工具链分离

### 📊 对比业界顶级标准的差距

参考对象：Zephyr RTOS、NuttX、STM32Cube 官方示例、Automotive AUTOSAR

## 改进建议（按优先级）

### 🔴 P0：关键改进（强烈推荐）

#### 1. **完全隔离 CubeMX 生成代码**

**当前问题**：
- `Inc/` 和 `Src/` 混在根目录，与用户代码边界不清
- `main.c` 和 `freertos.c` 在两个位置（`Src/` 和 `app/src/`），容易混淆
- CubeMX 重新生成需要手工合并

**改进方案**：
```
stm32f407/
├── cubemx/                    # CubeMX 生成的所有代码（隔离区）
│   ├── Core/
│   │   ├── Inc/              # 外设头文件（gpio.h, spi.h, ...）
│   │   └── Src/              # 外设源文件（gpio.c, spi.c, ...）
│   ├── Drivers/              # HAL 库（移入 cubemx/）
│   ├── Middlewares/          # FreeRTOS/LwIP（CubeMX 生成部分）
│   └── CubeMX_Config.ioc
├── bsp/                      # 用户板级驱动
├── app/                      # 用户应用代码
├── config/
├── linker/
└── startup/
```

**好处**：
- 用户代码与生成代码完全分离
- `app/src/main.c` 成为唯一的 main.c，调用 cubemx/ 中的初始化函数
- CubeMX 重新生成只影响 cubemx/ 目录

**实施难点**：
- 需要修改 CubeMX 项目输出路径（.ioc 文件或手工移动）
- CMakeLists.txt 需要大幅调整

---

#### 2. **应用层模块化拆分**

**当前问题**：
- `app/src/` 扁平化，所有应用代码在一个目录
- 缺少功能模块划分

**改进方案**：
```
app/
├── core/                      # 应用核心
│   ├── main.c                # 主程序入口
│   └── app_init.c            # 应用初始化
├── tasks/                     # FreeRTOS 任务
│   ├── task_lvgl.c           # LVGL 任务
│   ├── task_sensor.c         # 传感器任务
│   └── task_network.c        # 网络任务
├── ui/                        # 用户界面
│   ├── ui_main_screen.c
│   ├── ui_settings.c
│   └── ui_common.c
├── ports/                     # 移植层
│   ├── lv_port_disp.c
│   └── lv_port_indev.c
└── services/                  # 应用服务
    ├── data_logger.c
    └── event_dispatcher.c
```

**好处**：
- 按功能模块组织，易于查找和维护
- 多人协作时减少冲突
- 符合大型嵌入式项目规范

---

#### 3. **BSP 层板型化管理**

**当前问题**：
- `bsp/` 混合了硬件抽象和具体驱动
- 缺少板型配置（如果要支持多个开发板会很乱）

**改进方案**：
```
bsp/
├── common/                    # 通用 BSP 抽象层
│   ├── inc/
│   │   ├── bsp_lcd.h         # LCD 抽象接口
│   │   ├── bsp_touch.h       # 触摸抽象接口
│   │   └── bsp_uart.h        # 串口抽象接口
│   └── src/
│       └── (接口实现或为空)
├── boards/                    # 板型配置
│   └── alientek_explorer_v2.2/   # 正点原子探索者 V2.2
│       ├── board.h           # 板级配置（引脚定义、外设映射）
│       ├── board.c
│       └── README.md         # 板型说明
└── components/                # 可复用的外设驱动
    ├── lcd/
    │   ├── nt35510/          # NT35510 LCD 驱动
    │   │   ├── nt35510.c
    │   │   ├── nt35510.h
    │   │   └── nt35510_init.inc
    │   └── (其他 LCD 驱动)
    ├── touch/
    │   ├── gt9147/           # GT9147 触摸驱动
    │   │   ├── gt9147.c
    │   │   └── gt9147.h
    │   └── gt917s/           # GT917S（如果需要区分）
    └── uart/
        └── uart_dbg/
            ├── uart_dbg.c
            └── uart_dbg.h
```

**好处**：
- 驱动组件可复用到其他项目
- 支持多板型切换（通过 CMake 选项）
- 符合 Zephyr 的 board + driver 模型

---

### 🟡 P1：重要改进（推荐）

#### 4. **测试框架集成**

```
tests/
├── unit/                      # 单元测试
│   ├── test_lcd.c
│   ├── test_gt9147.c
│   └── CMakeLists.txt
├── integration/               # 集成测试
│   └── test_lvgl_touch.c
├── mocks/                     # HAL 层 mock
│   └── mock_hal_gpio.c
└── CMakeLists.txt
```

**工具**：Unity + CMock + Ceedling

---

#### 5. **脚本和工具集中化**

```
scripts/
├── flash.sh                   # ST-Link 烧录脚本
├── debug.sh                   # GDB 调试启动脚本
├── serial_monitor.py          # 串口监控工具
├── memory_analysis.py         # 内存占用分析
└── code_check.sh              # 静态分析（cppcheck/clang-tidy）
```

---

#### 6. **依赖管理规范化**

```
third_party/                   # 第三方库（只读）
├── lvgl/                      # LVGL（git submodule 或 release 包）
│   ├── src/
│   └── lvgl.h
├── FreeRTOS/                  # FreeRTOS
└── LwIP/                      # LwIP
```

**改进**：
- 使用 git submodule 管理第三方库版本
- 与 CubeMX Middlewares/ 解耦

---

### 🟢 P2：锦上添花（长期优化）

#### 7. **CI/CD 配置**

```
.github/
└── workflows/
    ├── build.yml              # 自动编译
    ├── test.yml               # 单元测试
    └── release.yml            # 发布固件
```

---

#### 8. **多环境配置**

```
config/
├── boards/                    # 板型配置
│   └── alientek_explorer_v2.2.cmake
├── profiles/                  # 编译配置文件
│   ├── debug.cmake
│   ├── release.cmake
│   └── test.cmake
└── (配置头文件)
```

---

#### 9. **版本管理和发布**

```
releases/
├── v1.0.0/
│   ├── firmware.bin
│   ├── firmware.elf
│   ├── memory_report.txt
│   └── CHANGELOG.md
└── latest/
```

---

## 实施路线图

### 阶段 1：立即改进（1-2天）

- [ ] 应用层模块化拆分（`app/tasks/`, `app/ui/`, `app/ports/`）
- [ ] BSP 组件化（`bsp/components/lcd/nt35510/`）
- [ ] 脚本集中化（`scripts/flash.sh`）

### 阶段 2：短期优化（1周）

- [ ] 完全隔离 CubeMX 代码到 `cubemx/` 目录
- [ ] 测试框架搭建（Unity + 基础单元测试）
- [ ] 第三方库 git submodule 化

### 阶段 3：中期规范（1个月）

- [ ] CI/CD 自动化构建
- [ ] 多板型支持框架
- [ ] 静态分析集成

### 阶段 4：长期演进（持续）

- [ ] 完整的单元测试覆盖
- [ ] 自动化硬件在环测试
- [ ] 发布管理流程

---

## 业界对标评分

### 当前结构评分（满分 10 分）

| 维度           | 当前得分 | 行业顶级 | 差距分析                     |
| ------------ | ---- | ---- | ------------------------ |
| 分层架构         | 7/10 | 10   | 缺少板型抽象和组件化               |
| 模块化          | 6/10 | 10   | 应用层扁平化，BSP 未组件化          |
| CubeMX 集成    | 5/10 | 9    | 生成代码与用户代码边界不清晰           |
| 配置管理         | 8/10 | 10   | 配置集中但缺少多环境支持             |
| 文档完善度        | 8/10 | 10   | 文档齐全，但缺少 API 文档          |
| 构建系统         | 7/10 | 10   | CMake 规范，但缺少多目标配置        |
| 测试框架         | 2/10 | 10   | 无单元测试，无自动化测试             |
| 工具链集成        | 6/10 | 10   | 缺少脚本和自动化工具               |
| CI/CD       | 0/10 | 10   | 无 CI/CD                  |
| **总分**       | **5.4/10** | **10** | **中等偏上，有明显提升空间** |

---

## 对比：Zephyr RTOS 项目结构

Zephyr 是业界顶级的嵌入式 RTOS，其项目结构：

```
zephyr/
├── arch/                      # CPU 架构层
├── boards/                    # 板型支持包（300+ 开发板）
├── drivers/                   # 设备驱动
├── include/                   # 公共头文件
├── kernel/                    # 内核源码
├── lib/                       # 标准库
├── modules/                   # 可选模块
├── samples/                   # 示例应用
├── scripts/                   # 工具脚本
├── subsys/                    # 子系统（文件系统/网络/USB）
├── tests/                     # 测试用例
└── CMakeLists.txt
```

**可借鉴的点**：
- boards/ 和 drivers/ 完全分离
- 每个板型有独立配置目录
- 丰富的 scripts/ 工具集
- 完善的 tests/ 框架

---

## 结论

### 当前结构评价

**优点**：
- ✅ 已达到小型专业项目标准（超过 90% 的个人/小团队项目）
- ✅ 分层清晰，配置集中，文档完善
- ✅ CubeMX 兼容性良好

**不足**：
- ⚠️ 缺少模块化和组件化（应用层扁平、BSP 组件未拆分）
- ⚠️ CubeMX 代码未完全隔离（与用户代码混合）
- ⚠️ 缺少测试框架和自动化工具
- ⚠️ 不支持多板型切换

### 建议

**如果项目是**：
- **个人学习/原型验证**：当前结构已足够 ✅
- **小团队（2-5人）短期项目**：实施 P0 中的 1-2 项即可
- **中型团队（5-10人）长期项目**：实施 P0 + P1 全部改进
- **商业产品/开源项目**：对标 Zephyr，实施全部改进

**立即可做的改进**（性价比最高）：
1. 应用层模块化拆分（`app/tasks/`, `app/ui/`, `app/ports/`）
2. BSP 组件化（`bsp/components/lcd/nt35510/`）
3. 添加 `scripts/flash.sh` 烧录脚本

要我帮你实施这些改进吗？
