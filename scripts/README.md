# 工具脚本说明

本目录包含 STM32 开发常用的自动化脚本。

## 脚本列表

### 1. `build_and_flash.sh` - 一键构建与烧录

**功能**：编译项目 → 内存分析 → 烧录固件（可选）

**用法**：
```bash
./scripts/build_and_flash.sh           # Debug 版本
./scripts/build_and_flash.sh Release   # Release 版本
```

**流程**：
1. 配置 CMake 项目
2. 编译（多核并行）
3. 显示内存占用统计
4. 询问是否烧录
5. 烧录到开发板

---

### 2. `flash.sh` - 固件烧录

**功能**：将编译好的固件烧录到开发板

**用法**：
```bash
./scripts/flash.sh           # 烧录 Debug 版本
./scripts/flash.sh Release   # 烧录 Release 版本
```

**支持的工具**：
- `st-flash` (stlink-tools)
- `STM32_Programmer_CLI` (STM32CubeProgrammer)

**安装烧录工具**：

**Linux**：
```bash
# 方法 1: stlink-tools
sudo apt install stlink-tools

# 方法 2: STM32CubeProgrammer
# 从 ST 官网下载: https://www.st.com/en/development-tools/stm32cubeprog.html
```

**Windows**：
- 下载 STM32CubeProgrammer 并添加到 PATH

---

### 3. `memory_analysis.sh` - 内存占用分析

**功能**：分析固件的 FLASH 和 RAM 占用情况

**用法**：
```bash
./scripts/memory_analysis.sh           # 分析 Debug 版本
./scripts/memory_analysis.sh Release   # 分析 Release 版本
```

**输出信息**：
- 基本内存占用（FLASH / RAM）
- 使用率百分比
- 各分段详情
- 占用最大的符号 Top 20
- 各模块内存占用（来自 MAP 文件）

---

### 4. `serial_monitor.sh` - 串口监控

**功能**：连接到 STM32 调试串口，查看输出日志

**用法**：
```bash
./scripts/serial_monitor.sh                    # 使用默认配置
./scripts/serial_monitor.sh /dev/ttyUSB0 115200   # Linux
./scripts/serial_monitor.sh COM18 115200      # Windows
```

**默认配置**：
- Linux: `/dev/ttyUSB0`
- Windows: `COM18`
- 波特率: `115200`

**支持的工具**：
- `minicom`
- `screen`
- `picocom`

**安装串口工具**：

**Linux**：
```bash
sudo apt install minicom    # 推荐
# 或
sudo apt install screen
# 或
sudo apt install picocom
```

**Windows**：
- 使用 PuTTY 或 TeraTerm

**退出快捷键**：
- `minicom`: `Ctrl+A` 然后 `X`
- `screen`: `Ctrl+A` 然后 `K`
- `picocom`: `Ctrl+A` 然后 `Ctrl+X`

---

## 典型开发流程

### 场景 1：首次编译并上板

```bash
# 1. 一键构建和烧录
./scripts/build_and_flash.sh

# 2. 监控串口输出
./scripts/serial_monitor.sh
```

### 场景 2：修改代码后重新测试

```bash
# 快速编译和烧录
./scripts/build_and_flash.sh

# 查看串口输出
./scripts/serial_monitor.sh
```

### 场景 3：Release 版本发布

```bash
# 编译 Release 版本
cmake --preset Release
cmake --build build/Release

# 分析内存占用
./scripts/memory_analysis.sh Release

# 烧录 Release 固件
./scripts/flash.sh Release
```

### 场景 4：仅查看串口输出（不烧录）

```bash
# 直接连接串口
./scripts/serial_monitor.sh
```

---

## 权限配置（Linux）

### USB 串口权限

如果遇到权限问题：

```bash
# 方法 1: 临时授权
sudo chmod 666 /dev/ttyUSB0

# 方法 2: 将用户加入 dialout 组（永久）
sudo usermod -a -G dialout $USER
# 注销后重新登录生效
```

### ST-Link 权限

创建 udev 规则：

```bash
# 创建规则文件
sudo nano /etc/udev/rules.d/99-stlink.rules

# 添加以下内容：
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666"

# 重新加载规则
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## 脚本使用权限（Git Bash / Windows）

所有脚本都使用 `#!/bin/bash`，可在以下环境运行：

- **Linux / macOS**: 直接运行
- **Windows**: 使用 Git Bash / MSYS2 / WSL

**添加执行权限**（如需要）：
```bash
chmod +x scripts/*.sh
```

---

## 故障排查

### 问题 1: 编译失败

**现象**：`build_and_flash.sh` 编译出错

**解决**：
```bash
# 检查工具链是否在 PATH 中
arm-none-eabi-gcc --version

# 清理后重新构建
rm -rf build/
cmake --preset Debug
cmake --build build/Debug
```

### 问题 2: 烧录失败

**现象**：`flash.sh` 无法连接到 ST-Link

**解决**：
1. 检查 ST-Link 是否连接：`st-info --probe`
2. 检查设备管理器（Windows）或 `lsusb`（Linux）
3. 尝试重新插拔 ST-Link
4. 检查开发板电源是否打开

### 问题 3: 串口无数据

**现象**：`serial_monitor.sh` 连接成功但无输出

**解决**：
1. 确认串口号正确（`ls /dev/ttyUSB*` 或设备管理器）
2. 确认波特率为 115200
3. 确认开发板已正常运行
4. 检查 USART1 引脚连接

### 问题 4: 权限被拒绝（Linux）

**现象**：`Permission denied: /dev/ttyUSB0`

**解决**：
```bash
# 临时方案
sudo chmod 666 /dev/ttyUSB0

# 永久方案
sudo usermod -a -G dialout $USER
# 注销后重新登录
```

---

## 进阶用法

### 自定义烧录脚本

如果使用 J-Link 等其他调试器，修改 `flash.sh` 中的 `flash_with_*` 函数。

### 集成到 IDE

**VS Code** 添加到 tasks.json：
```json
{
  "label": "Build and Flash",
  "type": "shell",
  "command": "${workspaceFolder}/scripts/build_and_flash.sh",
  "problemMatcher": []
}
```

### 自动化测试流程

```bash
# 编译 -> 烧录 -> 等待启动 -> 捕获串口日志
./scripts/build_and_flash.sh Debug
sleep 2
./scripts/serial_monitor.sh | tee test_output.log
```

---

## 参考

- [stlink-tools](https://github.com/stlink-org/stlink)
- [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
- [GNU ARM Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
