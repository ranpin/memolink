cmake_minimum_required(VERSION 3.10)
project(Mirix VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# 设置模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# 查找所需的库
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# 可选的FAISS支持
option(USE_FAISS "Use FAISS for vector search" OFF)
if(USE_FAISS)
    find_package(faiss REQUIRED)
endif()

# 查找JsonCpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 查找SQLite3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# 查找CURL
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)

# 查找Qt5（用于GUI）
find_package(Qt5 COMPONENTS Widgets Core Gui OPTIONAL_COMPONENTS)

# 收集源文件（更明确的路径）
file(GLOB_RECURSE MIRIX_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/**/*.cpp"
)

# 排除特定用途文件
list(FILTER MIRIX_SOURCES EXCLUDE REGEX ".*\\/(demo|test|main|api)_.*\\.cpp")
list(FILTER MIRIX_SOURCES EXCLUDE REGEX "src/main\\.cpp")

# 在macOS上包含Objective-C++文件
if(APPLE)
    file(GLOB_RECURSE OBJCPP_SOURCES "src/*.mm")
    list(APPEND MIRIX_SOURCES ${OBJCPP_SOURCES})
endif()

# 创建静态库
add_library(mirix_lib STATIC ${MIRIX_SOURCES})

# 添加包含目录
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})

# 添加链接目录
link_directories(${PORTAUDIO_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})
link_directories(${SQLITE3_LIBRARY_DIRS})
link_directories(${CURL_LIBRARY_DIRS})

# 添加编译选项
add_compile_options(${PORTAUDIO_CFLAGS_OTHER})
add_compile_options(${JSONCPP_CFLAGS_OTHER})
add_compile_options(${SQLITE3_CFLAGS_OTHER})
add_compile_options(${CURL_CFLAGS_OTHER})

# 条件编译定义
if(USE_FAISS)
    target_compile_definitions(mirix_lib PRIVATE USE_FAISS)
endif()

# 链接库
target_link_libraries(mirix_lib 
    ${OpenCV_LIBS}
    ${PORTAUDIO_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CURL_LIBRARIES}
    portaudio  # 直接链接portaudio库
)

# 如果启用了FAISS，则链接FAISS库
if(USE_FAISS)
    target_link_libraries(mirix_lib faiss)
endif()

# macOS 特定框架
if(APPLE)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
    find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IMAGE_IO_FRAMEWORK ImageIO)

    target_link_libraries(mirix_lib
        ${CORE_GRAPHICS_FRAMEWORK}
        ${CORE_FOUNDATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${AVFOUNDATION_FRAMEWORK}
        ${CORE_VIDEO_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${IMAGE_IO_FRAMEWORK}
    )

    # 确保.mm文件被视为C++
    if(TARGET mirix_lib AND OBJCPP_SOURCES)
        set_source_files_properties(${OBJCPP_SOURCES} PROPERTIES LANGUAGE CXX)
    endif()
endif()

# 创建可执行文件并链接
add_executable(mirix_main src/main.cpp)
target_link_libraries(mirix_main mirix_lib)

add_executable(comprehensive_demo src/comprehensive_demo.cpp)
target_link_libraries(comprehensive_demo mirix_lib)

add_executable(enhanced_comprehensive_demo src/enhanced_comprehensive_demo.cpp)
target_link_libraries(enhanced_comprehensive_demo mirix_lib)

add_executable(multimodal_demo src/multimodal_demo.cpp)
target_link_libraries(multimodal_demo mirix_lib)

add_executable(real_multimodal_demo src/real_multimodal_demo.cpp)
target_link_libraries(real_multimodal_demo mirix_lib)

add_executable(initialize_memories src/initialize_memories.cpp)
target_link_libraries(initialize_memories mirix_lib)

add_executable(test_embedding src/test_embedding_functionality.cpp)
target_link_libraries(test_embedding mirix_lib)

# GUI 可执行文件
if(Qt5_FOUND)
    add_executable(mirix_gui src/gui_main.cpp)
    target_link_libraries(mirix_gui mirix_lib Qt5::Widgets Qt5::Core Qt5::Gui)
    message(STATUS "Qt5 found, building GUI support")
else()
    message(STATUS "Qt5 not found, building without GUI support")
endif()