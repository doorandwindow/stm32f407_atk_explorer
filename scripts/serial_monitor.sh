#!/bin/bash
################################################################################
# 串口监控脚本
# 监控 STM32 调试串口输出
################################################################################

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# 默认配置（正点原子探索者 V2.2）
DEFAULT_PORT="/dev/ttyUSB0"  # Linux
DEFAULT_PORT_WIN="COM18"      # Windows
DEFAULT_BAUDRATE=115200

# 检测操作系统
detect_os() {
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        OS="Windows"
        DEFAULT_PORT="$DEFAULT_PORT_WIN"
    else
        OS="Linux"
    fi
}

# 检查串口工具
check_tool() {
    if command -v minicom &> /dev/null; then
        TOOL="minicom"
        echo -e "${GREEN}[✓]${NC} 使用 minicom"
        return 0
    elif command -v screen &> /dev/null; then
        TOOL="screen"
        echo -e "${GREEN}[✓]${NC} 使用 screen"
        return 0
    elif command -v picocom &> /dev/null; then
        TOOL="picocom"
        echo -e "${GREEN}[✓]${NC} 使用 picocom"
        return 0
    else
        echo -e "${RED}[✗]${NC} 未找到串口工具！"
        echo ""
        echo "请安装以下工具之一："
        echo "  - Linux: sudo apt install minicom"
        echo "  - Linux: sudo apt install screen"
        echo "  - Linux: sudo apt install picocom"
        echo "  - Windows: 使用 PuTTY 或 TeraTerm"
        return 1
    fi
}

# 列出可用串口
list_ports() {
    echo -e "${CYAN}可用串口:${NC}"
    if [[ "$OS" == "Windows" ]]; then
        # Windows: 列出 COM 口
        for i in {1..20}; do
            if [ -e "/dev/ttyS$i" ]; then
                echo "  COM$i"
            fi
        done
    else
        # Linux: 列出 USB 串口
        ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | while read port; do
            echo "  $port"
        done
    fi
    echo ""
}

# 启动串口监控
start_monitor() {
    local port="${1:-$DEFAULT_PORT}"
    local baudrate="${2:-$DEFAULT_BAUDRATE}"

    echo -e "${YELLOW}[→]${NC} 连接到 $port @ $baudrate bps"
    echo -e "${CYAN}提示: 按 Ctrl+A 然后 K 退出 (screen)${NC}"
    echo -e "${CYAN}提示: 按 Ctrl+A 然后 X 退出 (minicom)${NC}"
    echo -e "${CYAN}提示: 按 Ctrl+A 然后 Ctrl+X 退出 (picocom)${NC}"
    echo ""
    sleep 1

    case "$TOOL" in
        minicom)
            minicom -D "$port" -b "$baudrate"
            ;;
        screen)
            screen "$port" "$baudrate"
            ;;
        picocom)
            picocom -b "$baudrate" "$port"
            ;;
    esac
}

# 主流程
main() {
    detect_os

    echo "========================================"
    echo "  STM32 串口监控"
    echo "========================================"
    echo ""

    if ! check_tool; then
        exit 1
    fi

    list_ports

    local port="${1:-$DEFAULT_PORT}"
    local baudrate="${2:-$DEFAULT_BAUDRATE}"

    if [ ! -e "$port" ]; then
        echo -e "${RED}[✗]${NC} 串口 $port 不存在！"
        echo ""
        echo "用法: $0 [PORT] [BAUDRATE]"
        echo "示例: $0 /dev/ttyUSB0 115200"
        echo "示例: $0 COM18 115200"
        exit 1
    fi

    start_monitor "$port" "$baudrate"
}

# 帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "用法: $0 [PORT] [BAUDRATE]"
    echo ""
    echo "参数:"
    echo "  PORT      串口设备 (默认: $DEFAULT_PORT)"
    echo "  BAUDRATE  波特率 (默认: $DEFAULT_BAUDRATE)"
    echo ""
    echo "示例:"
    echo "  $0                           # 使用默认配置"
    echo "  $0 /dev/ttyUSB0 115200       # Linux"
    echo "  $0 COM18 115200              # Windows"
    echo ""
    echo "常用波特率: 9600, 19200, 38400, 57600, 115200"
    exit 0
fi

main "$@"
