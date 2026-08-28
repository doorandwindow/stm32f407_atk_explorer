#!/bin/bash
################################################################################
# STM32 烧录脚本
# 使用 ST-Link 将编译好的固件烧录到开发板
################################################################################

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Debug}"  # 默认 Debug，可通过参数指定 Release
ELF_FILE="$PROJECT_ROOT/build/$BUILD_TYPE/CubeMX_Config.elf"

# 检查 ST-Link 工具
check_stlink() {
    if command -v st-flash &> /dev/null; then
        FLASH_CMD="st-flash"
        echo -e "${GREEN}[✓]${NC} 找到 st-flash 工具"
        return 0
    elif command -v STM32_Programmer_CLI &> /dev/null; then
        FLASH_CMD="STM32_Programmer_CLI"
        echo -e "${GREEN}[✓]${NC} 找到 STM32_Programmer_CLI 工具"
        return 0
    else
        echo -e "${RED}[✗]${NC} 未找到烧录工具！"
        echo ""
        echo "请安装以下工具之一："
        echo "  1. stlink-tools: https://github.com/stlink-org/stlink"
        echo "  2. STM32CubeProgrammer: https://www.st.com/en/development-tools/stm32cubeprog.html"
        return 1
    fi
}

# 检查固件文件
check_firmware() {
    if [ ! -f "$ELF_FILE" ]; then
        echo -e "${RED}[✗]${NC} 固件文件不存在: $ELF_FILE"
        echo ""
        echo "请先编译项目："
        echo "  cmake --preset $BUILD_TYPE"
        echo "  cmake --build build/$BUILD_TYPE"
        return 1
    fi

    echo -e "${GREEN}[✓]${NC} 固件文件: $ELF_FILE"

    # 显示固件信息
    if command -v arm-none-eabi-size &> /dev/null; then
        echo ""
        echo "固件大小:"
        arm-none-eabi-size "$ELF_FILE"
    fi

    return 0
}

# 烧录固件（st-flash）
flash_with_stlink() {
    echo ""
    echo -e "${YELLOW}[→]${NC} 开始烧录..."

    # 转换 ELF 到 BIN
    BIN_FILE="${ELF_FILE%.elf}.bin"
    arm-none-eabi-objcopy -O binary "$ELF_FILE" "$BIN_FILE"

    # 烧录
    st-flash --reset write "$BIN_FILE" 0x08000000

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}[✓]${NC} 烧录成功！"
        rm -f "$BIN_FILE"
        return 0
    else
        echo -e "${RED}[✗]${NC} 烧录失败！"
        return 1
    fi
}

# 烧录固件（STM32_Programmer_CLI）
flash_with_cubeprog() {
    echo ""
    echo -e "${YELLOW}[→]${NC} 开始烧录..."

    STM32_Programmer_CLI -c port=SWD -w "$ELF_FILE" 0x08000000 -v -rst

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}[✓]${NC} 烧录成功！"
        return 0
    else
        echo -e "${RED}[✗]${NC} 烧录失败！"
        return 1
    fi
}

# 主流程
main() {
    echo "========================================"
    echo "  STM32 固件烧录脚本"
    echo "========================================"
    echo ""

    # 检查工具
    if ! check_stlink; then
        exit 1
    fi

    # 检查固件
    if ! check_firmware; then
        exit 1
    fi

    # 烧录
    if [ "$FLASH_CMD" = "st-flash" ]; then
        flash_with_stlink
    else
        flash_with_cubeprog
    fi

    echo ""
    echo "========================================"
}

# 显示帮助
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "用法: $0 [Debug|Release]"
    echo ""
    echo "选项:"
    echo "  Debug     烧录 Debug 版本固件（默认）"
    echo "  Release   烧录 Release 版本固件"
    echo ""
    echo "示例:"
    echo "  $0              # 烧录 Debug 版本"
    echo "  $0 Release      # 烧录 Release 版本"
    exit 0
fi

main
