#!/bin/bash

# MIRIX 依赖库设置脚本
# 支持 macOS 和 Linux 平台

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../" && pwd)"
LIBS_DIR="${PROJECT_ROOT}/libs"
INCLUDE_DIR="${LIBS_DIR}/include"
LIB_DIR="${LIBS_DIR}/lib"

# 创建目录
mkdir -p "${INCLUDE_DIR}" "${LIB_DIR}"

echo "Setting up dependencies for MIRIX..."

# 检测操作系统
OS="$(uname -s)"
case "${OS}" in
    Darwin*)
        echo "Detected macOS"
        setup_macos
        ;;
    Linux*)
        echo "Detected Linux"
        setup_linux
        ;;
    *)
        echo "Unsupported operating system: ${OS}"
        exit 1
        ;;
esac

setup_macos() {
    echo "Setting up dependencies for macOS..."
    
    # 检查 Homebrew
    if ! command -v brew &> /dev/null; then
        echo "Homebrew is not installed. Please install it from https://brew.sh/"
        exit 1
    fi
    
    # 安装依赖（如果尚未安装）
    brew install sqlite openssl curl jsoncpp portaudio faiss opencv
    
    # 复制依赖到本地目录
    copy_dependency "/opt/homebrew/Cellar/sqlite" "sqlite" "sqlite3"
    copy_dependency "/opt/homebrew/Cellar/openssl@3" "openssl" "ssl"
    copy_dependency "/opt/homebrew/Cellar/curl" "curl" "curl"
    copy_dependency "/opt/homebrew/Cellar/jsoncpp" "jsoncpp" "jsoncpp"
    copy_dependency "/opt/homebrew/Cellar/portaudio" "portaudio" "portaudio"
    copy_dependency "/opt/homebrew/Cellar/faiss" "faiss" "faiss"
    copy_opencv_libs "/opt/homebrew/opt/opencv"
    
    echo "Dependencies setup completed for macOS"
}

setup_linux() {
    echo "Setting up dependencies for Linux..."
    
    # 检查包管理器
    if command -v apt &> /dev/null; then
        PACKAGE_MANAGER="apt"
    elif command -v yum &> /dev/null; then
        PACKAGE_MANAGER="yum"
    elif command -v pacman &> /dev/null; then
        PACKAGE_MANAGER="pacman"
    else
        echo "Unsupported package manager. Please install dependencies manually."
        exit 1
    fi
    
    echo "Using package manager: ${PACKAGE_MANAGER}"
    
    # 安装依赖（如果尚未安装）
    case "${PACKAGE_MANAGER}" in
        apt)
            sudo apt update
            sudo apt install -y sqlite3 libsqlite3-dev libssl-dev libcurl4-openssl-dev \
                libjsoncpp-dev portaudio19-dev libfaiss-dev libopencv-dev
            ;;
        yum)
            sudo yum install -y sqlite-devel openssl-devel libcurl-devel \
                jsoncpp-devel portaudio-devel faiss-devel opencv-devel
            ;;
        pacman)
            sudo pacman -S --noconfirm sqlite openssl curl jsoncpp \
                portaudio faiss opencv
            ;;
    esac
    
    # 复制依赖到本地目录
    copy_dependency "/usr" "sqlite" "sqlite3"
    copy_dependency "/usr" "openssl" "ssl"
    copy_dependency "/usr" "curl" "curl"
    copy_dependency "/usr" "jsoncpp" "jsoncpp"
    copy_dependency "/usr" "portaudio" "portaudio"
    copy_dependency "/usr" "faiss" "faiss"
    copy_opencv_libs "/usr"
    
    echo "Dependencies setup completed for Linux"
}

copy_dependency() {
    local prefix="$1"
    local name="$2"
    local lib_name="$3"
    
    echo "Copying ${name}..."
    
    # 复制头文件
    if [ -d "${prefix}/include" ]; then
        cp -r "${prefix}/include"/* "${INCLUDE_DIR}/" 2>/dev/null || true
    fi
    
    # 复制库文件
    if [ -d "${prefix}/lib" ]; then
        find "${prefix}/lib" -name "lib${lib_name}.*" -exec cp {} "${LIB_DIR}/" \; 2>/dev/null || true
    fi
    
    # 对于某些库，可能在lib64目录中
    if [ -d "${prefix}/lib64" ]; then
        find "${prefix}/lib64" -name "lib${lib_name}.*" -exec cp {} "${LIB_DIR}/" \; 2>/dev/null || true
    fi
}

copy_opencv_libs() {
    local prefix="$1"
    
    echo "Copying OpenCV libraries..."
    
    # 复制头文件
    if [ -d "${prefix}/include/opencv4" ]; then
        cp -r "${prefix}/include/opencv4"/* "${INCLUDE_DIR}/" 2>/dev/null || true
    fi
    
    # 复制库文件
    local lib_dirs=("${prefix}/lib" "${prefix}/lib64" "${prefix}/lib/x86_64-linux-gnu")
    for lib_dir in "${lib_dirs[@]}"; do
        if [ -d "${lib_dir}" ]; then
            find "${lib_dir}" -name "libopencv_*" -exec cp {} "${LIB_DIR}/" \; 2>/dev/null || true
        fi
    done
}

echo "MIRIX dependencies setup script completed!"
echo "You can now build the project with:"
echo "  cd build && cmake .. && make"