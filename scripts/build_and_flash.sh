#!/bin/bash
################################################################################
# 一键构建并烧录脚本
# 编译项目并自动烧录到开发板
################################################################################

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# 配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Debug}"

# 步骤 1: 构建
build_project() {
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  步骤 1/3: 配置项目${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    cd "$PROJECT_ROOT"
    cmake --preset "$BUILD_TYPE"

    echo ""
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  步骤 2/3: 编译项目${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    cmake --build "build/$BUILD_TYPE" -j$(nproc 2>/dev/null || echo 4)

    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}[✓]${NC} 编译成功！"
    else
        echo -e "${RED}[✗]${NC} 编译失败！"
        exit 1
    fi
}

# 步骤 2: 内存分析
analyze_memory() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  内存占用分析${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    "$SCRIPT_DIR/memory_analysis.sh" "$BUILD_TYPE" | head -20
}

# 步骤 3: 烧录
flash_firmware() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  步骤 3/3: 烧录固件${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    "$SCRIPT_DIR/flash.sh" "$BUILD_TYPE"
}

# 主流程
main() {
    echo "========================================"
    echo "  STM32 一键构建与烧录"
    echo "========================================"
    echo ""
    echo "构建类型: $BUILD_TYPE"
    echo ""

    build_project
    analyze_memory

    echo ""
    read -p "是否立即烧录？ [Y/n] " -n 1 -r
    echo ""

    if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]; then
        flash_firmware
        echo ""
        echo -e "${GREEN}[✓]${NC} 全部完成！"
    else
        echo ""
        echo -e "${YELLOW}[!]${NC} 已跳过烧录"
    fi

    echo ""
    echo "========================================"
}

# 帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "用法: $0 [Debug|Release]"
    echo ""
    echo "功能:"
    echo "  1. 配置并编译项目"
    echo "  2. 分析内存占用"
    echo "  3. 烧录到开发板（可选）"
    echo ""
    echo "参数:"
    echo "  Debug     构建 Debug 版本 (默认)"
    echo "  Release   构建 Release 版本"
    echo ""
    echo "示例:"
    echo "  $0              # Debug 版本"
    echo "  $0 Release      # Release 版本"
    exit 0
fi

main
