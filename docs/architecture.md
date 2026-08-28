# 项目架构说明

## 目录结构

```
stm32f407/
├── CMakeLists.txt              # 主构建文件
├── CMakePresets.json           # CMake 预设配置
├── README.md                   # 项目说明
├── CLAUDE.md                   # Claude Code 工作指南
├── .gitignore
│
├── config/                     # 配置文件集中管理
│   ├── lv_conf.h              # LVGL 配置
│   ├── FreeRTOSConfig.h       # FreeRTOS 配置
│   ├── lwipopts.h             # LwIP 配置
│   └── stm32f4xx_hal_conf.h   # HAL 库配置
│
├── linker/                     # 链接脚本
│   └── STM32F407xx_FLASH.ld
│
├── startup/                    # 启动文件
│   └── startup_stm32f407xx.s
│
├── Inc/                        # CubeMX 生成的头文件（自动生成，勿手工修改）
├── Src/                        # CubeMX 生成的源文件（自动生成，勿手工修改）
├── CubeMX_Config.ioc          # CubeMX 配置文件
│
├── bsp/                        # Board Support Package（板级驱动）
│   ├── inc/
│   │   ├── lcd.h              # NT35510 LCD 驱动头文件
│   │   ├── gt9147.h           # GT9147/GT917S 触摸驱动头文件
│   │   └── uart_dbg.h         # 调试串口头文件
│   └── src/
│       ├── lcd.c              # LCD 驱动实现
│       ├── lcd_nt35510_init.inc  # LCD 初始化序列
│       ├── gt9147.c           # 触摸驱动实现
│       └── uart_dbg.c         # 调试串口实现
│
├── app/                        # 应用层代码
│   ├── inc/                   # 应用层头文件（预留）
│   └── src/
│       ├── main.c             # 主程序（包含 USER CODE 区）
│       ├── freertos.c         # FreeRTOS 任务定义
│       ├── lv_port_disp.c     # LVGL 显示接口
│       └── lv_port_indev.c    # LVGL 输入设备接口
│
├── Drivers/                    # ST 官方驱动库（只读）
│   ├── CMSIS/                 # ARM CMSIS 标准接口
│   ├── STM32F4xx_HAL_Driver/  # STM32 HAL 库
│   └── BSP/                   # 板级支持包组件
│
├── Middlewares/                # 中间件
│   └── Third_Party/
│       ├── FreeRTOS/          # FreeRTOS 实时操作系统
│       ├── LwIP/              # LwIP 网络协议栈
│       └── lvgl/              # LVGL 图形库
│
├── docs/                       # 文档
│   ├── architecture.md        # 本文件：架构说明
│   └── debug-loop.md          # 上板调试闭环说明
│
└── cmake/                      # CMake 模块
    ├── gcc-arm-none-eabi.cmake  # 工具链配置
    └── stm32cubemx/
        └── CMakeLists.txt      # CubeMX 生成代码的构建脚本
```

## 分层说明

### 1. CubeMX 生成层（`Inc/`、`Src/`）
- **作用**：硬件外设初始化代码（GPIO、UART、SPI、FSMC、IWDG 等）
- **特点**：由 CubeMX 自动生成，重新生成时会覆盖
- **约束**：用户代码只能写在 `/* USER CODE BEGIN/END */` 标记区内
- **迁移说明**：
  - `main.c` 和 `freertos.c` 已迁移到 `app/src/`，但仍保留 USER CODE 区
  - 其他外设初始化文件（`gpio.c`、`fsmc.c` 等）保留在原位置

### 2. BSP 层（`bsp/`）
- **作用**：板级硬件驱动，与开发板强相关
- **包含**：
  - LCD 驱动（NT35510）
  - 触摸驱动（GT9147/GT917S）
  - 调试串口（uart_dbg，用于 `dbg_printf`）
- **特点**：可移植到其他使用相同硬件的项目

### 3. 应用层（`app/`）
- **作用**：应用逻辑和任务编排
- **包含**：
  - `main.c`：主程序入口（保留 CubeMX USER CODE 区）
  - `freertos.c`：FreeRTOS 任务定义
  - `lv_port_*.c`：LVGL 移植接口
- **特点**：与具体应用业务相关

### 4. 驱动层（`Drivers/`）
- **作用**：ST 官方提供的 HAL 库和 CMSIS
- **特点**：只读，通过 CubeMX 更新

### 5. 中间件层（`Middlewares/`）
- **作用**：第三方软件组件
- **包含**：FreeRTOS、LwIP、LVGL
- **特点**：独立于硬件平台

### 6. 配置层（`config/`）
- **作用**：集中管理所有配置头文件
- **包含**：LVGL、FreeRTOS、LwIP、HAL 的配置文件
- **优势**：便于查找和修改配置

## CubeMX 兼容性

### 重新生成代码时的注意事项

1. **保护 `app/src/main.c` 和 `app/src/freertos.c`**：
   - 这两个文件已迁移到 `app/src/`，CubeMX 会在 `Src/` 重新生成它们
   - 重新生成后，需要手工将 USER CODE 区的改动合并到 `app/src/` 版本
   - 或者临时将 `app/src/main.c` 改名，让 CubeMX 生成后再合并

2. **配置文件路径**：
   - `config/` 中的配置文件是复制品，原件仍在 `Inc/`
   - CubeMX 重新生成时会更新 `Inc/` 中的配置，需同步到 `config/`
   - 未来考虑：修改 CubeMX 输出路径直接指向 `config/`

3. **启动文件和链接脚本**：
   - 已迁移到 `startup/` 和 `linker/`
   - CubeMX 会在根目录重新生成，需手工同步到对应目录

### 推荐工作流程

1. 修改 `CubeMX_Config.ioc` 并重新生成代码
2. 检查 `Src/main.c` 和 `Src/freertos.c` 的 USER CODE 区是否有变化
3. 如有变化，合并到 `app/src/` 对应文件
4. 检查 `Inc/` 中的配置文件是否更新，同步到 `config/`
5. 检查根目录的 `startup_stm32f407xx.s` 和 `STM32F407xx_FLASH.ld`，同步到 `startup/` 和 `linker/`
6. 重新编译验证

## 构建系统

### CMake 路径配置

- **根目录 `CMakeLists.txt`**：
  - 引入 BSP 和应用层源文件
  - 配置 LVGL 源码路径
  - 添加 `config/`、`bsp/inc/`、`app/inc/` 到头文件搜索路径

- **`cmake/stm32cubemx/CMakeLists.txt`**：
  - 管理 CubeMX 生成的源文件
  - 包含 HAL 库和中间件
  - 从 `app/src/` 引用 `main.c` 和 `freertos.c`
  - 从 `startup/` 引用启动文件

- **`cmake/gcc-arm-none-eabi.cmake`**：
  - 工具链配置
  - 从 `linker/` 引用链接脚本

### 编译命令

```bash
cmake --preset Debug            # 配置（首次或改 CMakeLists 后）
cmake --build build/Debug       # 编译
cmake --build build/Release     # Release 构建（-Os）
```

## 内存布局

参考 `docs/debug-loop.md` 中的「RAM 布局速查」章节。

## 未来改进方向

1. **完全分离 CubeMX 代码**：
   - 将 `Inc/`、`Src/` 移到 `cubemx/` 目录
   - 修改 CubeMX 输出路径设置

2. **添加单元测试框架**：
   - 在 `tests/` 目录下搭建测试环境
   - 使用 Unity 或 Ceedling

3. **自动化上板测试**：
   - 集成 ST-Link 烧录脚本
   - 自动化串口日志捕获

4. **应用层模块化**：
   - 将 `app/src/` 按功能拆分子目录（ui/、tasks/、drivers/）
