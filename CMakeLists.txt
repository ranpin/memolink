cmake_minimum_required(VERSION 3.10)
project(MIRIX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 构建形态开关（orin-edge 分支）
# ----------------------------------------------------------------------------
# 端侧记忆系统默认只编核心：database / models / agents / utils / storage。
# 桌面专属部分（多模态采集 + CLIP、Qt GUI、HTTP API server + CLI）默认关闭，
# **代码一律保留不删**，需要时用开关打开：
#
#   端侧（Orin，默认）：  cmake ..
#   桌面（Mac，全功能）：  cmake .. -DMIRIX_ENABLE_CAPTURE=ON \
#                                   -DMIRIX_ENABLE_GUI=ON -DMIRIX_ENABLE_API=ON
#
# 关掉 CAPTURE 同时也就不需要 OpenCV / PortAudio；关掉 GUI 不需要 Qt5。
# 这样端侧构建的依赖面只剩 sqlite3 + curl + jsoncpp（+ EDGE 的 onnxruntime）。
# ============================================================================
option(MIRIX_ENABLE_CAPTURE "相机/音频/截屏采集 + CLIP 多模态（需 OpenCV、PortAudio）" OFF)
option(MIRIX_ENABLE_GUI     "Qt GUI（需 Qt5 Widgets/Core/Gui）" OFF)
option(MIRIX_ENABLE_API     "HTTP API server + 命令行交互（需 CAPTURE 提供的多模态入口）" OFF)
option(MIRIX_ENABLE_EDGE    "Orin 端侧本地推理（ONNX Runtime C API）" ON)

# ---------- 必需依赖 ----------
find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)
if(NOT jsoncpp_FOUND)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

# 注：OpenSSL 曾是 REQUIRED，但全仓 grep `openssl/` 零命中（HTTPS 由 libcurl 内部处理），
# 故不再要求——少一个端侧构建依赖。

# ---------- 可选依赖（随开关）----------
if(MIRIX_ENABLE_CAPTURE)
    find_package(OpenCV REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
endif()

if(MIRIX_ENABLE_GUI)
    find_package(Qt5 COMPONENTS Widgets Core Gui REQUIRED)
endif()

# ---------- 端侧推理：ONNX Runtime（官方 release 包，含 C 头 + 库）----------
if(MIRIX_ENABLE_EDGE)
    set(MIRIX_ONNXRUNTIME_ROOT "$ENV{HOME}/chenrunbin/deps/onnxruntime-linux-aarch64-1.31.0"
        CACHE PATH "onnxruntime release 解压根目录（含 include/ 与 lib/）")
    set(_ort_api_header "${MIRIX_ONNXRUNTIME_ROOT}/include/onnxruntime/core/session/onnxruntime_c_api.h")
    if(NOT EXISTS "${_ort_api_header}")
        message(FATAL_ERROR
            "MIRIX_ENABLE_EDGE=ON 但找不到 ${_ort_api_header}\n"
            "请下载 onnxruntime linux-aarch64 release 并用 "
            "-DMIRIX_ONNXRUNTIME_ROOT=<解压目录> 指定，或 -DMIRIX_ENABLE_EDGE=OFF。")
    endif()
    include_directories("${MIRIX_ONNXRUNTIME_ROOT}/include/onnxruntime/core/session")
    link_directories("${MIRIX_ONNXRUNTIME_ROOT}/lib")
endif()

# ---------- include / link 目录 ----------
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})

if(MIRIX_ENABLE_CAPTURE)
    include_directories(${PORTAUDIO_INCLUDE_DIRS})
    include_directories(${OpenCV_INCLUDE_DIRS})
    link_directories(${PORTAUDIO_LIBRARY_DIRS})
    link_directories(${OpenCV_LIBRARY_DIRS})
endif()

# ============================================================================
# 源文件
# ----------------------------------------------------------------------------
# 注：向量检索是对 SQLite 中存储的 embedding 做暴力余弦相似度
# （见 src/models/VectorSearchManager.cpp），并未链接 FAISS。
# EmbeddingIndexManager 里的 FAISS 方法仍是占位符；F2 实测已证明在 agent 记忆
# 量级（1e3~1e4）下精确检索足够（0.79ms@1万条、recall 1.0），而 HNSW 建索引要
# 36s@10万条，故本项目刻意不接 FAISS（连带不需要 Eigen3）。
# ============================================================================
set(SOURCES
    # ---- 数据库核心 ----
    src/database/core/StorageManager.cpp
    src/database/core/TransactionManager.cpp
    src/database/initialization/InitDB.cpp
    src/database/initialization/ConfigManager/ConfigManager.cpp
    # ---- 各类型记忆 manager（记忆系统本体，2618 行）----
    src/database/managers/UserManager.cpp
    src/database/managers/SessionManager.cpp
    src/database/managers/CoreMemoryBlockManager.cpp
    src/database/managers/EpisodicMemoryManager.cpp
    src/database/managers/SemanticMemoryManager.cpp
    src/database/managers/ProceduralMemoryManager.cpp
    src/database/managers/ResourceMemoryManager.cpp
    src/database/managers/KnowledgeVaultManager.cpp
    src/database/managers/ArchivalMemoryManager.cpp
    src/database/managers/RecallMemoryManager.cpp
    src/database/managers/BackupManager.cpp
    src/database/managers/AuditLogManager.cpp
    src/database/managers/VisualizationManager.cpp
    src/database/managers/MemoryOrchestrator.cpp
    # ---- 模型接口 ----
    src/models/ModelManager.cpp
    src/models/QwenAPI.cpp
    src/models/TextEmbeddingGenerator.cpp
    src/models/VectorSearchManager.cpp
    src/models/EmbeddingIndexManager.cpp
    # ---- Agent ----
    src/agents/AgentManager.cpp
    src/agents/AgentWrapper.cpp
    src/agents/ChatAgent.cpp
    src/agents/CoreMemoryAgent.cpp
    src/agents/EpisodicMemoryAgent.cpp
    src/agents/SemanticMemoryAgent.cpp
    src/agents/ProceduralMemoryAgent.cpp
    src/agents/ResourceMemoryAgent.cpp
    src/agents/KnowledgeVaultAgent.cpp
    # ---- 工具 / 存储 ----
    src/utils/VectorSerializer.cpp
    src/utils/TimeUtils.cpp
    src/storage/StorageManager.cpp
)

# ---- 桌面专属：多模态采集 + CLIP（需 OpenCV / PortAudio）----
if(MIRIX_ENABLE_CAPTURE)
    list(APPEND SOURCES
        src/models/CLIPProcessor.cpp
        src/capture/CameraCaptureManager.cpp
        src/capture/AudioCaptureManager.cpp
        src/capture/ScreenActivityTracker.cpp
        src/capture/SystemEventManager.cpp
        src/capture/MultimodalDataManager.cpp
    )
endif()

# ---- 桌面专属：HTTP API server + CLI ----
# CommandLineInterface.h include 了 ../api/APIServer.h，故 CLI 随 API 一起开关。
if(MIRIX_ENABLE_API)
    list(APPEND SOURCES
        src/api/APIServer.cpp
        src/cli/CommandLineInterface.cpp
    )
endif()

# ---- 桌面专属：Qt GUI ----
if(MIRIX_ENABLE_GUI)
    list(APPEND SOURCES src/gui/GUIManager.cpp)
endif()

# ---- Objective-C++（仅 macOS 截屏，属 CAPTURE）----
if(APPLE AND MIRIX_ENABLE_CAPTURE)
    file(GLOB_RECURSE OBJCPP_SOURCES "src/capture/*.mm")
    if(OBJCPP_SOURCES)
        list(APPEND SOURCES ${OBJCPP_SOURCES})
    endif()
endif()

# ============================================================================
# 静态库
# ============================================================================
add_library(mirix_lib STATIC ${SOURCES})

# VectorSearchManager 里的 CLIP 回退路径用这个宏隔离（CLIPProcessor 依赖 OpenCV
# 且通过 popen 调 Python，端侧构建不需要也不应该有）
if(MIRIX_ENABLE_CAPTURE)
    target_compile_definitions(mirix_lib PUBLIC MIRIX_HAS_CLIP=1)
endif()
if(MIRIX_ENABLE_EDGE)
    target_compile_definitions(mirix_lib PUBLIC MIRIX_HAS_EDGE=1)
endif()

target_link_libraries(mirix_lib
    SQLite::SQLite3
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    jsoncpp
)
if(MIRIX_ENABLE_CAPTURE)
    target_link_libraries(mirix_lib ${PORTAUDIO_LIBRARIES} ${OpenCV_LIBS})
endif()
if(MIRIX_ENABLE_EDGE)
    target_link_libraries(mirix_lib onnxruntime)
endif()

if(APPLE AND MIRIX_ENABLE_CAPTURE)
    if(OBJCPP_SOURCES)
        set_source_files_properties(${OBJCPP_SOURCES} PROPERTIES LANGUAGE CXX)
    endif()
    # 屏幕捕获需要 ApplicationServices
    target_link_libraries(mirix_lib "-framework ApplicationServices")
endif()

# ============================================================================
# 可执行文件
# ----------------------------------------------------------------------------
# 不依赖采集/CLIP 的测试始终构建；依赖的随 MIRIX_ENABLE_CAPTURE 开关。
# ============================================================================
# ⚠️ 有两个目标名与源文件名不一致，不能靠 test/${t}.cpp 拼：
#      test_embedding       ← test/test_embedding_functionality.cpp
#      real_multimodal_demo ← test/test_real_multimodal_demo.cpp
#    所以这两个显式写映射，其余同名的才用 foreach。

# ---- 始终构建（不依赖采集 / CLIP）----
set(MIRIX_ALWAYS_TESTS
    test_vector_serializer
    test_system_initialization
    test_initialize_memories
    test_embedding_comprehensive
    test_chat_with_model
)
foreach(t ${MIRIX_ALWAYS_TESTS})
    add_executable(${t} test/${t}.cpp)
    target_link_libraries(${t} mirix_lib)
endforeach()

# ---- 依赖多模态采集 / CLIP（随 MIRIX_ENABLE_CAPTURE 开关）----
if(MIRIX_ENABLE_CAPTURE)
    add_executable(test_embedding test/test_embedding_functionality.cpp)
    target_link_libraries(test_embedding mirix_lib)

    add_executable(test_clip_processor test/test_clip_processor.cpp)
    target_link_libraries(test_clip_processor mirix_lib)

    add_executable(test_chat_with_auto_capture test/test_chat_with_auto_capture.cpp)
    target_link_libraries(test_chat_with_auto_capture mirix_lib)

    add_executable(real_multimodal_demo test/test_real_multimodal_demo.cpp)
    target_link_libraries(real_multimodal_demo mirix_lib)

    set(MIRIX_CAPTURE_TESTS
        test_embedding
        test_clip_processor
        test_chat_with_auto_capture
        real_multimodal_demo
    )
endif()

# GUI 可执行文件
# 注：GUI 入口 src/gui_main.cpp 与 MacOSXBundleInfo.plist.in 目前并不存在
# （src/gui/ 下只有 GUIManager.cpp/.h），补上之后再启用这段。
# if(MIRIX_ENABLE_GUI)
#     add_executable(mirix_gui WIN32 src/gui_main.cpp)
#     target_link_libraries(mirix_gui mirix_lib Qt5::Widgets Qt5::Core Qt5::Gui)
#     if(APPLE)
#         set_target_properties(mirix_gui PROPERTIES
#             MACOSX_BUNDLE TRUE
#             MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/src/gui/MacOSXBundleInfo.plist.in
#         )
#     endif()
# endif()

# ============================================================================
# 安装规则
# ============================================================================
install(TARGETS mirix_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(TARGETS ${MIRIX_ALWAYS_TESTS}
    RUNTIME DESTINATION bin
)
if(MIRIX_ENABLE_CAPTURE)
    install(TARGETS ${MIRIX_CAPTURE_TESTS}
        RUNTIME DESTINATION bin
    )
endif()

install(DIRECTORY src/
    DESTINATION include/mirix
    FILES_MATCHING PATTERN "*.h"
)
install(FILES config.ini DESTINATION etc)

# ============================================================================
# 测试
# ============================================================================
enable_testing()
foreach(t ${MIRIX_ALWAYS_TESTS})
    add_test(NAME ${t} COMMAND ${t})
endforeach()
if(MIRIX_ENABLE_CAPTURE)
    foreach(t ${MIRIX_CAPTURE_TESTS})
        add_test(NAME ${t} COMMAND ${t})
    endforeach()
endif()
