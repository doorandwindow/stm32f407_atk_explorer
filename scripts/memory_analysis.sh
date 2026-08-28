#!/bin/bash
################################################################################
# 内存分析脚本
# 分析编译产物的内存占用情况
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
ELF_FILE="$PROJECT_ROOT/build/$BUILD_TYPE/CubeMX_Config.elf"
MAP_FILE="$PROJECT_ROOT/build/$BUILD_TYPE/CubeMX_Config.map"

# STM32F407ZGT6 规格
FLASH_SIZE=1048576   # 1MB
RAM_SIZE=131072      # 128KB
CCMRAM_SIZE=65536    # 64KB

# 检查文件
check_files() {
    if [ ! -f "$ELF_FILE" ]; then
        echo -e "${RED}[✗]${NC} ELF 文件不存在: $ELF_FILE"
        return 1
    fi

    if [ ! -f "$MAP_FILE" ]; then
        echo -e "${YELLOW}[!]${NC} MAP 文件不存在，将仅显示基本信息"
        MAP_FILE=""
    fi

    return 0
}

# 基本内存信息
show_basic_info() {
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  基本内存占用${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    arm-none-eabi-size "$ELF_FILE"
    echo ""

    # 详细分析
    local text=$(arm-none-eabi-size "$ELF_FILE" | awk 'NR==2 {print $1}')
    local data=$(arm-none-eabi-size "$ELF_FILE" | awk 'NR==2 {print $2}')
    local bss=$(arm-none-eabi-size "$ELF_FILE" | awk 'NR==2 {print $3}')

    local flash_used=$((text + data))
    local ram_used=$((data + bss))

    local flash_percent=$((flash_used * 100 / FLASH_SIZE))
    local ram_percent=$((ram_used * 100 / RAM_SIZE))

    echo -e "${GREEN}FLASH 使用:${NC}"
    echo -e "  总量: $FLASH_SIZE 字节 (1024 KB)"
    echo -e "  已用: $flash_used 字节 ($flash_percent%)"
    echo -e "  可用: $((FLASH_SIZE - flash_used)) 字节"
    echo ""

    echo -e "${GREEN}RAM 使用:${NC}"
    echo -e "  总量: $RAM_SIZE 字节 (128 KB)"
    echo -e "  已用: $ram_used 字节 ($ram_percent%)"
    echo -e "  可用: $((RAM_SIZE - ram_used)) 字节"
    echo ""

    # 警告
    if [ $flash_percent -gt 90 ]; then
        echo -e "${RED}[!] 警告: FLASH 使用率超过 90%！${NC}"
    fi

    if [ $ram_percent -gt 90 ]; then
        echo -e "${RED}[!] 警告: RAM 使用率超过 90%！${NC}"
    fi
}

# 分段信息
show_sections() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  分段详情${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    arm-none-eabi-objdump -h "$ELF_FILE" | grep -E "^\s+[0-9]+" | \
    awk '{printf "  %-20s %10s bytes  (VMA: 0x%s)\n", $2, "0x" $3, $4}'
}

# 符号占用 Top 20
show_top_symbols() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  占用最大的符号 (Top 20)${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    arm-none-eabi-nm --print-size --size-sort --radix=d "$ELF_FILE" | \
    tail -20 | \
    awk '{printf "  %8d bytes  %-10s  %s\n", $2, $3, $4}' | \
    tac
}

# MAP 文件分析
analyze_map() {
    if [ -z "$MAP_FILE" ]; then
        return
    fi

    echo ""
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo -e "${CYAN}  模块内存占用 (来自 MAP 文件)${NC}"
    echo -e "${CYAN}═══════════════════════════════════════${NC}"
    echo ""

    # 提取每个 .o 文件的大小
    grep -A 1000 "Memory Configuration" "$MAP_FILE" | \
    grep "\.o" | \
    awk '{print $1, $2}' | \
    sort -k2 -rn | \
    head -20 | \
    awk '{printf "  %8s  %s\n", $2, $1}'
}

# 主流程
main() {
    echo "========================================"
    echo "  STM32 内存分析"
    echo "========================================"
    echo ""
    echo "构建类型: $BUILD_TYPE"
    echo "ELF 文件: $ELF_FILE"
    echo ""

    if ! check_files; then
        exit 1
    fi

    show_basic_info
    show_sections
    show_top_symbols
    analyze_map

    echo ""
    echo "========================================"
}

# 帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "用法: $0 [Debug|Release]"
    echo ""
    echo "参数:"
    echo "  Debug     分析 Debug 版本 (默认)"
    echo "  Release   分析 Release 版本"
    echo ""
    echo "示例:"
    echo "  $0              # 分析 Debug 版本"
    echo "  $0 Release      # 分析 Release 版本"
    exit 0
fi

main
