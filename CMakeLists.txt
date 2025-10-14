# This is a sample CMakeLists.txt file for a C++ project.

cmake_minimum_required(VERSION 3.10)

project(MirixCpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(MirixCpp src/main.cpp src/config/ConfigManager.cpp src/storage/StorageManager.cpp)