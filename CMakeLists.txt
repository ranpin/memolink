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
# jsoncpp：find_package(jsoncpp) 成功时只给 imported target、**不设 JSONCPP_INCLUDE_DIRS**，
# 而源码用 #include <json/json.h>，需要 /usr/include/jsoncpp 在 include 路径上
# （Ubuntu 的 jsoncpp.pc 正是给 -I/usr/include/jsoncpp）。故统一走 pkg-config。
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

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
    # 官方 release 包是**平铺**布局：include/onnxruntime_c_api.h
    # 源码树才是 include/onnxruntime/core/session/onnxruntime_c_api.h —— 两种都兼容。
    set(_ort_inc "${MIRIX_ONNXRUNTIME_ROOT}/include")
    if(NOT EXISTS "${_ort_inc}/onnxruntime_c_api.h")
        if(EXISTS "${_ort_inc}/onnxruntime/core/session/onnxruntime_c_api.h")
            set(_ort_inc "${_ort_inc}/onnxruntime/core/session")
        else()
            message(FATAL_ERROR
                "MIRIX_ENABLE_EDGE=ON 但找不到 onnxruntime_c_api.h\n"
                "  已查 ${MIRIX_ONNXRUNTIME_ROOT}/include/onnxruntime_c_api.h\n"
                "  以及 ${MIRIX_ONNXRUNTIME_ROOT}/include/onnxruntime/core/session/onnxruntime_c_api.h\n"
                "请下载 onnxruntime linux-aarch64 release，用 -DMIRIX_ONNXRUNTIME_ROOT=<解压目录> 指定，"
                "或 -DMIRIX_ENABLE_EDGE=OFF。")
        endif()
    endif()
    message(STATUS "onnxruntime headers: ${_ort_inc}")
    include_directories("${_ort_inc}")
    # release 包的 include/core/ 下还有被上面那些头 include 的文件
    if(IS_DIRECTORY "${MIRIX_ONNXRUNTIME_ROOT}/include/core")
        include_directories("${MIRIX_ONNXRUNTIME_ROOT}/include/core")
    endif()
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
    src/database/managers/EdgeMemoryStore.cpp
    src/database/managers/MemoryResolver.cpp
    src/database/managers/MemoryExtractor.cpp
    # ---- 模型接口 ----
    src/models/ModelManager.cpp
    src/models/QwenAPI.cpp
    src/models/TextEmbeddingGenerator.cpp
    src/models/VectorSearchManager.cpp
    src/models/EmbeddingIndexManager.cpp
    src/models/WordPieceTokenizer.cpp
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

# ---- 端侧推理：ONNX Runtime 原生 C++ API（随 MIRIX_ENABLE_EDGE）----
if(MIRIX_ENABLE_EDGE)
    list(APPEND SOURCES src/models/OnnxEmbedder.cpp)
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
# 注：test/test_initialize_memories.cpp 没有 main()（既有缺陷，main 分支的 CMake
#     也一直把它当可执行文件构建、实际从未链接成功），故不纳入构建目标。
set(MIRIX_ALWAYS_TESTS
    test_vector_serializer
    test_system_initialization
    test_embedding_comprehensive
    test_chat_with_model
    test_tokenizer_parity
)
foreach(t ${MIRIX_ALWAYS_TESTS})
    add_executable(${t} test/${t}.cpp)
    target_link_libraries(${t} mirix_lib)
endforeach()

# 端侧 ONNX embedding 对拍工具 + TextEmbeddingGenerator 接线验证（依赖 onnxruntime，随 EDGE 开关）
if(MIRIX_ENABLE_EDGE)
    add_executable(test_embed_parity test/test_embed_parity.cpp)
    target_link_libraries(test_embed_parity mirix_lib)

    add_executable(test_edge_embedding test/test_edge_embedding.cpp)
    target_link_libraries(test_edge_embedding mirix_lib)

    # P3：记忆编排（写入/跨类型检索/敏感标记/实时写入/分类型 FIFO 淘汰）
    add_executable(test_memory_orchestrator test/test_memory_orchestrator.cpp)
    target_link_libraries(test_memory_orchestrator mirix_lib)

    # P4：三域带标注检索评测（recall@k / MRR / hit@1），与 LLM 生成解耦
    add_executable(test_retrieval_eval test/test_retrieval_eval.cpp)
    target_link_libraries(test_retrieval_eval mirix_lib)

    # Q1：v2 记忆存储（独立 memory 表 + 显式 CRUD + 非破坏性版本化）
    add_executable(test_edge_memory_store test/test_edge_memory_store.cpp)
    target_link_libraries(test_edge_memory_store mirix_lib)

    # Q2：五操作消解（三层判定 + System-1 覆盖率 + 五分类准确率）
    add_executable(test_memory_resolver test/test_memory_resolver.cpp)
    target_link_libraries(test_memory_resolver mirix_lib)

    # Q3：类型特化检索 vs 单一策略基线（hit@1 对照 + 意图识别准确率）
    add_executable(test_typed_retrieval test/test_typed_retrieval.cpp)
    target_link_libraries(test_typed_retrieval mirix_lib)

    # Q5：LongMemEval turn 级检索评测（recall@k / MRR），英文模型 config-only swap
    add_executable(test_longmemeval_retrieval test/test_longmemeval_retrieval.cpp)
    target_link_libraries(test_longmemeval_retrieval mirix_lib)

    # Q6：mirix-mem CLI——八命令串完整管线（提取→消解→存储→检索→更新→删除→历史→统计）
    add_executable(mirix-mem src/cli/MemoryCLI.cpp)
    target_link_libraries(mirix-mem mirix_lib)
endif()

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
