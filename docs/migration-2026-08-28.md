# 项目结构重构记录（2026-08-28）

## 重构目标

将项目从 CubeMX 生成的扁平结构重构为专业化的分层架构，符合嵌入式工程最佳实践。

## 变更概览

### 新增目录

| 目录 | 用途 | 内容 |
|------|------|------|
| `config/` | 配置文件集中管理 | `lv_conf.h`, `FreeRTOSConfig.h`, `lwipopts.h`, `stm32f4xx_hal_conf.h` |
| `linker/` | 链接脚本 | `STM32F407xx_FLASH.ld` |
| `startup/` | 启动文件 | `startup_stm32f407xx.s` |
| `bsp/` | BSP 层（板级驱动） | `bsp/inc/` 和 `bsp/src/` |
| `app/` | 应用层 | `app/inc/` 和 `app/src/` |
| `docs/` | 文档 | `architecture.md`, `debug-loop.md`, `migration-2026-08-28.md` |

### 文件迁移映射

#### BSP 层文件

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `Inc/lcd.h` | `bsp/inc/lcd.h` | LCD 驱动头文件 |
| `Inc/gt9147.h` | `bsp/inc/gt9147.h` | 触摸驱动头文件 |
| `Inc/uart_dbg.h` | `bsp/inc/uart_dbg.h` | 调试串口头文件 |
| `Src/lcd.c` | `bsp/src/lcd.c` | LCD 驱动实现 |
| `Src/lcd_nt35510_init.inc` | `bsp/src/lcd_nt35510_init.inc` | LCD 初始化序列 |
| `Src/gt9147.c` | `bsp/src/gt9147.c` | 触摸驱动实现 |
| `Src/uart_dbg.c` | `bsp/src/uart_dbg.c` | 调试串口实现 |

#### 应用层文件

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `Src/main.c` | `app/src/main.c` | 主程序（包含 USER CODE 区） |
| `Src/freertos.c` | `app/src/freertos.c` | FreeRTOS 任务定义 |
| `Src/lv_port_disp.c` | `app/src/lv_port_disp.c` | LVGL 显示接口 |
| `Src/lv_port_indev.c` | `app/src/lv_port_indev.c` | LVGL 输入设备接口 |

#### 配置文件

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `lv_conf.h` | `config/lv_conf.h` | LVGL 配置 |
| `Inc/FreeRTOSConfig.h` | `config/FreeRTOSConfig.h` | FreeRTOS 配置 |
| `Inc/lwipopts.h` | `config/lwipopts.h` | LwIP 配置 |
| `Inc/stm32f4xx_hal_conf.h` | `config/stm32f4xx_hal_conf.h` | HAL 库配置 |

#### 启动和链接文件

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `STM32F407xx_FLASH.ld` | `linker/STM32F407xx_FLASH.ld` | 链接脚本 |
| `startup_stm32f407xx.s` | `startup/startup_stm32f407xx.s` | 启动文件 |

#### 文档文件

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `debug-loop.md` | `docs/debug-loop.md` | 上板调试闭环说明 |
| N/A | `docs/architecture.md` | 架构说明（新增） |
| N/A | `docs/migration-2026-08-28.md` | 本迁移记录（新增） |

### 构建系统变更

#### `CMakeLists.txt`（根目录）

**修改前：**
```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Src/lcd.c
    Src/gt9147.c
    Src/uart_dbg.c
    Src/lv_port_disp.c
    Src/lv_port_indev.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/lvgl
)
```

**修改后：**
```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # BSP layer
    bsp/src/lcd.c
    bsp/src/gt9147.c
    bsp/src/uart_dbg.c

    # Application layer
    app/src/lv_port_disp.c
    app/src/lv_port_indev.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Configuration files
    ${CMAKE_CURRENT_SOURCE_DIR}/config

    # BSP layer includes
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/inc

    # Application layer includes
    ${CMAKE_CURRENT_SOURCE_DIR}/app/inc

    # LVGL middleware
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/lvgl

    # Root (for backward compatibility)
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

#### `cmake/stm32cubemx/CMakeLists.txt`

**主要变更：**

1. **头文件搜索路径添加**：
```cmake
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../config          # 新增
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/inc         # 新增
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/inc         # 新增
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LwIP/src/include
    ...
)
```

2. **应用源文件路径修改**：
```cmake
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/main.c      # 从 Src/main.c 改为 app/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/freertos.c  # 从 Src/freertos.c 改为 app/src/freertos.c
    ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup/startup_stm32f407xx.s  # 从根目录改为 startup/
)
```

#### `cmake/gcc-arm-none-eabi.cmake`

**链接脚本路径修改：**
```cmake
# 修改前
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F407xx_FLASH.ld\"")

# 修改后
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/linker/STM32F407xx_FLASH.ld\"")
```

### 文档更新

1. **README.md**：更新工程结构章节，添加 `docs/architecture.md` 链接
2. **CLAUDE.md**：更新架构章节，说明新的目录结构和 CubeMX 兼容性
3. **新增 docs/architecture.md**：完整的架构说明文档
4. **迁移 debug-loop.md**：从根目录移到 `docs/`

## 迁移策略

采用**渐进式迁移**，确保不影响现有功能：

1. ✅ 创建新目录结构（`bsp/`, `app/`, `config/`, `linker/`, `startup/`, `docs/`）
2. ✅ 复制文件到新位置（保留原文件不删除）
3. ✅ 修改 CMakeLists.txt 引用新路径
4. ✅ 修改工具链配置文件
5. ✅ 验证编译通过
6. ⬜ 上板测试验证功能正常
7. ⬜ 删除原位置的冗余文件（可选，暂时保留以便回退）

## 验证结果

### 编译验证

```bash
$ cmake --preset Debug
Build type: Debug
-- Configuring done (0.2s)
-- Generating done (0.1s)
-- Build files have been written to: D:/video_my/stm32_ai/stm32f407/build/Debug

$ cmake --build build/Debug
[346/346] Linking C executable CubeMX_Config.elf
Memory region         Used Size  Region Size  %age Used
             RAM:      109444 B       128 KB     83.54%
           FLASH:      478420 B      1024 KB     45.65%
```

**结果：编译成功 ✅**

### 上板验证

⬜ 待完成

## 兼容性说明

### CubeMX 重新生成代码时的注意事项

1. **`Src/main.c` 和 `Src/freertos.c`**：
   - 这两个文件已迁移到 `app/src/`
   - CubeMX 重新生成时会在 `Src/` 重新创建它们
   - 需要手工将 USER CODE 区的改动合并到 `app/src/` 版本

2. **配置文件**：
   - `config/` 中的配置文件是复制品，原件仍在 `Inc/`
   - CubeMX 重新生成时会更新 `Inc/` 中的配置
   - 需要手工同步到 `config/`

3. **启动文件和链接脚本**：
   - CubeMX 会在根目录重新生成
   - 需要手工同步到 `startup/` 和 `linker/`

### 推荐工作流程

当需要修改 CubeMX 配置时：

1. 修改 `CubeMX_Config.ioc` 并重新生成代码
2. 检查 `Src/main.c` 和 `Src/freertos.c` 的 USER CODE 区是否有变化
3. 如有变化，合并到 `app/src/` 对应文件
4. 检查 `Inc/` 中的配置文件是否更新，同步到 `config/`
5. 检查根目录的启动文件和链接脚本，同步到 `startup/` 和 `linker/`
6. 重新编译验证

## 好处

### 1. 分层清晰
- **CubeMX 生成层**：`Inc/`, `Src/` 专门放 CubeMX 生成的代码
- **BSP 层**：板级驱动独立，可复用到其他项目
- **应用层**：业务逻辑与驱动分离
- **配置层**：所有配置文件集中管理，便于查找

### 2. 便于维护
- 新增驱动放 `bsp/`，不会和 CubeMX 代码混淆
- 应用逻辑放 `app/`，职责明确
- 配置文件统一在 `config/`，不用在多个目录查找

### 3. 符合业界规范
- 参考 Zephyr、STM32Cube 示例工程的目录规范
- 更容易被其他开发者理解
- 便于多人协作

### 4. 扩展性强
- 未来可以轻松添加更多 BSP 组件
- 应用层可以按模块拆分子目录
- 便于集成测试框架

## 后续改进方向

1. **完全分离 CubeMX 代码**：
   - 将 `Inc/`, `Src/` 移到 `cubemx/` 目录
   - 修改 CubeMX 输出路径设置（需研究 CubeMX 是否支持）

2. **删除冗余文件**：
   - 上板验证通过后，删除根目录的旧文件
   - 删除 `Inc/` 和 `Src/` 中已迁移的用户文件

3. **应用层模块化**：
   - 将 `app/src/` 按功能拆分子目录（`ui/`, `tasks/`, `ports/`）

4. **添加单元测试**：
   - 在 `tests/` 目录搭建测试框架
   - 使用 Unity 或 Ceedling

## 相关文档

- [架构说明](architecture.md)
- [调试闭环说明](debug-loop.md)
- [根目录 README.md](../README.md)
- [根目录 CLAUDE.md](../CLAUDE.md)
