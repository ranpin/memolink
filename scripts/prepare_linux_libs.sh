#!/bin/bash

# prepare_linux_libs.sh - 为Linux系统准备库文件的脚本

echo "为Linux系统准备库文件"
echo "========================"

# 创建必要的目录结构
echo "创建目录结构..."
mkdir -p ../libs/linux/include
mkdir -p ../libs/linux/lib

echo "目录结构创建完成:"
echo "libs/linux/"
echo "├── include/"
echo "└── lib/"

echo ""
echo "在Linux系统上，建议使用系统包管理器安装依赖:"
echo "Ubuntu/Debian:"
echo "  sudo apt-get update"
echo "  sudo apt-get install libsqlite3-dev libssl-dev libcurl4-openssl-dev"
echo "  sudo apt-get install libjsoncpp-dev libportaudio19-dev"
echo "  sudo apt-get install libopencv-dev"
echo "  sudo apt-get install libfaiss-dev"

echo ""
echo "CentOS/RHEL/Fedora:"
echo "  sudo yum install sqlite-devel openssl-devel curl-devel"
echo "  sudo yum install jsoncpp-devel portaudio-devel"
echo "  sudo yum install opencv-devel"
echo "  sudo yum install faiss-devel"

echo ""
echo "或者使用以下命令下载预编译库文件到相应目录:"
echo "  cp /usr/lib/x86_64-linux-gnu/libsqlite3.so* ../libs/linux/lib/"
echo "  cp /usr/lib/x86_64-linux-gnu/libssl.so* ../libs/linux/lib/"
echo "  cp /usr/lib/x86_64-linux-gnu/libcrypto.so* ../libs/linux/lib/"
echo "  cp /usr/lib/x86_64-linux-gnu/libcurl.so* ../libs/linux/lib/"
echo "  cp /usr/lib/x86_64-linux-gnu/libjsoncpp.so* ../libs/linux/lib/"
echo "  cp /usr/lib/x86_64-linux-gnu/libportaudio.so* ../libs/linux/lib/"
echo "  cp /usr/lib/x86_64-linux-gnu/libopencv_*.so* ../libs/linux/lib/"

echo ""
echo "复制头文件到include目录:"
echo "  cp -r /usr/include/sqlite3.h ../libs/linux/include/"
echo "  cp -r /usr/include/openssl ../libs/linux/include/"
echo "  cp -r /usr/include/curl ../libs/linux/include/"
echo "  cp -r /usr/include/jsoncpp ../libs/linux/include/"
echo "  cp -r /usr/include/portaudio.h ../libs/linux/include/"
echo "  cp -r /usr/include/opencv4/opencv2 ../libs/linux/include/"

echo ""
echo "设置完成后，使用以下命令构建项目:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make"