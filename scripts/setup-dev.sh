#!/usr/bin/env bash
# ===========================================================================
# setup-dev.sh — Profex 开发环境一键配置
# 用法: sudo ./scripts/setup-dev.sh
#
# 自动安装构建 Profex 所需的系统依赖。
# 支持: Debian/Ubuntu, Fedora/RHEL, macOS (Homebrew)
# ===========================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}   $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
err()   { echo -e "${RED}[ERR]${NC}  $*"; }

detect_os() {
    case "$(uname -s)" in
        Linux)
            if grep -qi ubuntu /etc/os-release 2>/dev/null; then
                OS="ubuntu"
            elif grep -qi debian /etc/os-release 2>/dev/null; then
                OS="debian"
            elif grep -qi fedora /etc/os-release 2>/dev/null; then
                OS="fedora"
            else
                OS="linux"
            fi
            ;;
        Darwin)
            OS="macos"
            ;;
        *)
            err "不支持的操作系统: $(uname -s)"
            exit 1
            ;;
    esac
    info "检测到操作系统: $OS"
}

setup_apt() {
    info "更新包列表..."
    apt-get update -qq

    info "安装构建工具..."
    apt-get install -y -qq \
        build-essential \
        cmake \
        pkg-config \
        git

    info "安装 Qt5 及依赖..."
    apt-get install -y -qq \
        qtbase5-dev \
        qt5-qmake \
        qttools5-dev-tools \
        qtdeclarative5-dev \
        libqt5svg5-dev \
        libqt5xml5 \
        zlib1g-dev

    info "安装文档工具 (可选)..."
    apt-get install -y -qq \
        doxygen \
        graphviz \
        python3 \
        python3-pip \
        2>/dev/null || true

    ok "依赖安装完成"
}

setup_fedora() {
    info "安装构建工具..."
    dnf install -y \
        gcc-c++ make cmake pkgconf-pkg-config

    info "安装 Qt5..."
    dnf install -y \
        qt5-qtbase-devel \
        qt5-qttools-devel \
        qt5-qtdeclarative-devel \
        qt5-qtsvg-devel \
        zlib-devel

    info "安装文档工具 (可选)..."
    dnf install -y doxygen graphviz python3-pip 2>/dev/null || true

    ok "依赖安装完成"
}

setup_macos() {
    if ! command -v brew &>/dev/null; then
        err "需要 Homebrew: https://brew.sh/"
        exit 1
    fi

    info "安装 Qt5..."
    brew install qt@5

    info "安装其他工具..."
    brew install cmake pkg-config doxygen graphviz python3

    info "设置 Qt5 环境变量..."
    echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc
    echo 'export PATH="/usr/local/opt/qt@5/bin:$PATH"' >> ~/.bashrc

    ok "依赖安装完成。请重新打开终端或执行: source ~/.zshrc"
}

main() {
    echo "=============================="
    echo " Profex Dev Environment Setup"
    echo "=============================="
    echo ""

    detect_os

    case "$OS" in
        ubuntu|debian)
            setup_apt
            ;;
        fedora)
            setup_fedora
            ;;
        macos)
            setup_macos
            ;;
        *)
            err "请手动安装依赖: Qt5, g++, cmake, zlib"
            exit 1
            ;;
    esac

    # 安装 Python 工具 (MCP Server 准备)
    if command -v pip3 &>/dev/null; then
        info "安装 Python MCP SDK..."
        pip3 install --quiet mcp 2>/dev/null || warn "mcp SDK 安装失败 (可忽略，后续手动安装)"
    fi

    echo ""
    ok "开发环境配置完成！"
    echo "  运行构建: bash scripts/build.sh"
    echo "  生成文档: cd $PROJECT_DIR && doxygen Doxyfile"
}

main
