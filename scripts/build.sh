#!/usr/bin/env bash
# ===========================================================================
# build.sh — Profex 一键构建脚本
# 用法: ./scripts/build.sh [--clean] [--debug] [--cmake]
#
# 自动检测 Qt 版本，支持 qmake 和 cmake 两种构建方式。
# 保持核心源码不变，纯外围构建脚本。
# ===========================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
JOBS="${JOBS:-$(nproc)}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}   $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
err()   { echo -e "${RED}[ERR]${NC}  $*"; }

# 解析参数
CLEAN=false
DEBUG=false
USE_CMAKE=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)    CLEAN=true; shift ;;
        --debug)    DEBUG=true; shift ;;
        --cmake)    USE_CMAKE=true; shift ;;
        --help|-h)  echo "用法: $0 [--clean] [--debug] [--cmake]"; exit 0 ;;
        *)          err "未知参数: $1"; exit 1 ;;
    esac
done

# 检测 Qt
detect_qt() {
    if command -v qmake &>/dev/null; then
        QMAKE=$(command -v qmake)
    elif command -v qmake6 &>/dev/null; then
        QMAKE=$(command -v qmake6)
    elif command -v qmake-qt5 &>/dev/null; then
        QMAKE=$(command -v qmake-qt5)
    fi

    if command -v cmake &>/dev/null; then
        CMAKE=$(command -v cmake)
    fi

    if [[ -z "${QMAKE:-}" && -z "${CMAKE:-}" ]]; then
        err "未检测到 Qt (qmake) 或 CMake！"
        err "请安装: sudo apt install qtbase5-dev qt5-qmake cmake"
        exit 1
    fi

    if [[ -n "${QMAKE:-}" ]]; then
        QT_VER=$("$QMAKE" -query QT_VERSION 2>/dev/null || echo "unknown")
        info "检测到 qmake ($QT_VER): $QMAKE"
    fi
    if [[ -n "${CMAKE:-}" ]]; then
        info "检测到 cmake: $CMAKE"
    fi
}

# 安装依赖检测
check_deps() {
    local missing=()
    for cmd in g++ make pkg-config; do
        if ! command -v "$cmd" &>/dev/null; then
            missing+=("$cmd")
        fi
    done
    if [[ ${#missing[@]} -gt 0 ]]; then
        err "缺少依赖: ${missing[*]}"
        err "请安装: sudo apt install build-essential pkg-config"
        exit 1
    fi
}

# qmake 构建
build_qmake() {
    info "使用 qmake 构建..."

    if [[ "$CLEAN" == true ]]; then
        info "清理 qmake 构建缓存..."
        rm -f "$PROJECT_DIR"/.qmake.stash
        for dir in "$PROJECT_DIR"/zlib "$PROJECT_DIR"/quazip "$PROJECT_DIR"/libXrdIO "$PROJECT_DIR"/profex; do
            make -C "$dir" clean 2>/dev/null || true
        done
        ok "清理完成"
    fi

    cd "$PROJECT_DIR"

    local qmake_args=""
    if [[ "$DEBUG" == true ]]; then
        qmake_args="CONFIG+=debug"
    fi

    "$QMAKE" $qmake_args profex.pro
    make -j"$JOBS"
}

# cmake 构建
build_cmake() {
    info "使用 CMake 构建..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    if [[ "$CLEAN" == true ]]; then
        info "清理 CMake 构建..."
        rm -rf "$BUILD_DIR"/*
    fi

    local cmake_args="-DCMAKE_BUILD_TYPE="
    if [[ "$DEBUG" == true ]]; then
        cmake_args+="Debug"
    else
        cmake_args+="Release"
    fi

    "${CMAKE:-cmake}" "$PROJECT_DIR" $cmake_args
    cmake --build . -j"$JOBS"
}

# 主流程
main() {
    echo "=============================="
    echo " Profex Build Script"
    echo "=============================="
    echo ""

    check_deps
    detect_qt

    if [[ "$USE_CMAKE" == true ]]; then
        if [[ -z "${CMAKE:-}" ]]; then
            use_cmake=false
            warn "CMake 不可用，回退到 qmake"
        fi
    fi

    if [[ "$USE_CMAKE" == true && -n "${CMAKE:-}" ]]; then
        build_cmake
    else
        build_qmake
    fi

    echo ""
    if [[ -f "$PROJECT_DIR/profex/profex" ]]; then
        ok "构建成功！可执行文件: $PROJECT_DIR/profex/profex"
    elif [[ -f "$BUILD_DIR/profex/profex" ]]; then
        ok "构建成功！可执行文件: $BUILD_DIR/profex/profex"
    else
        warn "构建可能已完成，未找到可执行文件（部分模块可能不可用）。"
    fi
}

main
