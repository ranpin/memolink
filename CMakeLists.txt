cmake_minimum_required(VERSION 3.10)
project(memolink)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 构建形态（orin-edge 分支）
# ----------------------------------------------------------------------------
# 端侧记忆系统只编活跃四层：app（CLI）/ memory（api + core）/ infra（embedding +
# llm + storage）。旧 MIRIX 桌面系统（多模态采集 + CLIP、Qt GUI、HTTP API、9 个
# agent、12 个 v1 记忆 manager）已于 2026-10-22 整体移入 src/legacy/ 并**移出
# 构建**——代码留盘仅作参考，不再编译进 memolink_lib。详见 docs/architecture.md
# 「v1 设计遗产与未来方向」。
#
#   端侧（Orin，默认）：  cmake ..                          # MEMOLINK_ENABLE_EDGE=ON
#   纯逻辑（无 ONNX）：    cmake .. -DMEMOLINK_ENABLE_EDGE=OFF
#
# 端侧构建依赖面：sqlite3 + curl + jsoncpp + onnxruntime（aarch64）。
# ============================================================================
option(MEMOLINK_ENABLE_EDGE "Orin 端侧本地推理（ONNX Runtime C API）" ON)

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

# ---------- 端侧推理：ONNX Runtime（官方 release 包，含 C 头 + 库）----------
if(MEMOLINK_ENABLE_EDGE)
    set(MEMOLINK_ONNXRUNTIME_ROOT "$ENV{HOME}/chenrunbin/deps/onnxruntime-linux-aarch64-1.31.0"
        CACHE PATH "onnxruntime release 解压根目录（含 include/ 与 lib/）")
    # 官方 release 包是**平铺**布局：include/onnxruntime_c_api.h
    # 源码树才是 include/onnxruntime/core/session/onnxruntime_c_api.h —— 两种都兼容。
    set(_ort_inc "${MEMOLINK_ONNXRUNTIME_ROOT}/include")
    if(NOT EXISTS "${_ort_inc}/onnxruntime_c_api.h")
        if(EXISTS "${_ort_inc}/onnxruntime/core/session/onnxruntime_c_api.h")
            set(_ort_inc "${MEMOLINK_ONNXRUNTIME_ROOT}/include/onnxruntime/core/session")
        else()
            message(FATAL_ERROR
                "MEMOLINK_ENABLE_EDGE=ON 但找不到 onnxruntime_c_api.h\n"
                "  已查 ${MEMOLINK_ONNXRUNTIME_ROOT}/include/onnxruntime_c_api.h\n"
                "  以及 ${MEMOLINK_ONNXRUNTIME_ROOT}/include/onnxruntime/core/session/onnxruntime_c_api.h\n"
                "请下载 onnxruntime linux-aarch64 release，用 -DMEMOLINK_ONNXRUNTIME_ROOT=<解压目录> 指定，"
                "或 -DMEMOLINK_ENABLE_EDGE=OFF。")
        endif()
    endif()
    message(STATUS "onnxruntime headers: ${_ort_inc}")
    include_directories("${_ort_inc}")
    # release 包的 include/core/ 下还有被上面那些头 include 的文件
    if(IS_DIRECTORY "${MEMOLINK_ONNXRUNTIME_ROOT}/include/core")
        include_directories("${MEMOLINK_ONNXRUNTIME_ROOT}/include/core")
    endif()
    link_directories("${MEMOLINK_ONNXRUNTIME_ROOT}/lib")
endif()

# ---------- include / link 目录 ----------
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})

# ============================================================================
# 源文件（活跃四层：app / memory / infra；legacy 不参与构建）
# ----------------------------------------------------------------------------
# 注：向量检索是对 SQLite 中存储的 embedding 做暴力余弦相似度（见
# src/memory/core/MemoryStore.cpp 的 searchByEmbedding），并未链接 FAISS。
# F2 实测已证明在 agent 记忆量级（1e3~1e4）下精确检索足够（0.79ms@1万条、
# recall 1.0），而 HNSW 建索引要 36s@10万条，故本项目刻意不接 FAISS（连带不
# 需要 Eigen3）。
# ============================================================================
set(SOURCES
    # ---- infra/storage ----
    src/infra/storage/StorageManager.cpp
    src/infra/storage/TransactionManager.cpp
    src/infra/storage/InitDB.cpp
    src/infra/storage/ConfigManager.cpp
    src/infra/storage/VectorSerializer.cpp
    src/infra/storage/TimeUtils.cpp
    # ---- infra/llm + embedding ----
    src/infra/llm/QwenAPI.cpp
    src/infra/embedding/TextEmbeddingGenerator.cpp
    src/infra/embedding/WordPieceTokenizer.cpp
    # ---- memory/core（记忆系统本体）----
    src/memory/core/MemoryStore.cpp
    src/memory/core/MemoryResolver.cpp
    src/memory/core/MemoryExtractor.cpp
    # ---- memory/api（MemorySystem 门面）----
    src/memory/api/MemorySystem.cpp
)

# ---- 端侧推理：ONNX Runtime 原生 C++ API（随 MEMOLINK_ENABLE_EDGE）----
if(MEMOLINK_ENABLE_EDGE)
    list(APPEND SOURCES src/infra/embedding/OnnxEmbedder.cpp)
endif()

# ============================================================================
# 静态库
# ============================================================================
add_library(memolink_lib STATIC ${SOURCES})

if(MEMOLINK_ENABLE_EDGE)
    target_compile_definitions(memolink_lib PUBLIC MEMOLINK_HAS_EDGE=1)
endif()

target_link_libraries(memolink_lib
    SQLite::SQLite3
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    jsoncpp
)
if(MEMOLINK_ENABLE_EDGE)
    target_link_libraries(memolink_lib onnxruntime)
endif()

# ============================================================================
# 可执行文件与测试（全部 v2、legacy-free）
# ----------------------------------------------------------------------------
# Q1–Q7 闸门测试 + 端侧 embedding 基础设施测试 + memolink CLI。
# 旧 v1/P3-P4 时代依赖 legacy 的测试（test_memory_orchestrator /
# test_retrieval_eval / test_system_initialization / test_embedding_comprehensive /
# test_chat_with_model 及 CAPTURE 系列）已随 legacy 移出构建，文件留盘于 test/。
# ============================================================================

# ---- 始终构建（不依赖 onnxruntime）----
set(MEMOLINK_ALWAYS_TESTS
    test_vector_serializer
    test_tokenizer_parity
)
foreach(t ${MEMOLINK_ALWAYS_TESTS})
    add_executable(${t} test/${t}.cpp)
    target_link_libraries(${t} memolink_lib)
endforeach()

# ---- 端侧（依赖 onnxruntime，随 EDGE 开关）----
if(MEMOLINK_ENABLE_EDGE)
    # 端侧 ONNX embedding 对拍工具 + TextEmbeddingGenerator 接线验证
    add_executable(test_embed_parity test/test_embed_parity.cpp)
    target_link_libraries(test_embed_parity memolink_lib)

    add_executable(test_edge_embedding test/test_edge_embedding.cpp)
    target_link_libraries(test_edge_embedding memolink_lib)

    # Q1：v2 记忆存储（独立 memory 表 + 显式 CRUD + 非破坏性版本化）
    add_executable(test_edge_memory_store test/test_edge_memory_store.cpp)
    target_link_libraries(test_edge_memory_store memolink_lib)

    # Q2：五操作消解（三层判定 + System-1 覆盖率 + 五分类准确率）
    add_executable(test_memory_resolver test/test_memory_resolver.cpp)
    target_link_libraries(test_memory_resolver memolink_lib)

    # Q3：类型特化检索 vs 单一策略基线（hit@1 对照 + 意图识别准确率）
    add_executable(test_typed_retrieval test/test_typed_retrieval.cpp)
    target_link_libraries(test_typed_retrieval memolink_lib)

    # Q5：LongMemEval turn 级检索评测（recall@k / MRR），英文模型 config-only swap
    add_executable(test_longmemeval_retrieval test/test_longmemeval_retrieval.cpp)
    target_link_libraries(test_longmemeval_retrieval memolink_lib)

    # Q7：治理三元标签过滤 + 强度衰减淘汰（episodic 先淘汰 / procedural 最后）
    add_executable(test_memory_governance test/test_memory_governance.cpp)
    target_link_libraries(test_memory_governance memolink_lib)

    # Q9：authority 参与消解决策（observed > asserted > inferred，低不得静默覆盖高）
    #     + source_image 视觉溯源列。A/B 部分是纯函数、无需模型即可验证闸门真值表。
    add_executable(test_multimodal_authority test/test_multimodal_authority.cpp)
    target_link_libraries(test_multimodal_authority memolink_lib)

    # Q9 · M2 端到端：真实「System-2（本地 LLM）→ 闸门 → 落库」链路（需 :8099 qwen3-4b）
    add_executable(test_authority_e2e test/test_authority_e2e.cpp)
    target_link_libraries(test_authority_e2e memolink_lib)

    # Q6：memolink CLI——八命令串完整管线（提取→消解→存储→检索→更新→删除→历史→统计）
    add_executable(memolink src/app/cli/MemoryCLI.cpp)
    target_link_libraries(memolink memolink_lib)

    # 座舱助手——app 层第二个公民（带记忆的座舱助手；演示 SUPERSEDE 版本化 /
    # episodic·procedural 类型特化检索 / 多驾驶员治理拦截）。只依赖 MemorySystem 门面。
    add_executable(cockpit_assistant
        src/app/cockpit/CockpitAssistant.cpp
        src/app/cockpit/cockpit_main.cpp)
    target_link_libraries(cockpit_assistant memolink_lib)
endif()

# ============================================================================
# 安装规则
# ============================================================================
install(TARGETS memolink_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(TARGETS ${MEMOLINK_ALWAYS_TESTS}
    RUNTIME DESTINATION bin
)
if(MEMOLINK_ENABLE_EDGE)
    install(TARGETS memolink RUNTIME DESTINATION bin)
endif()

install(DIRECTORY src/
    DESTINATION include/memolink
    FILES_MATCHING PATTERN "*.h"
)
install(FILES config.ini DESTINATION etc)

# ============================================================================
# 测试
# ============================================================================
enable_testing()
foreach(t ${MEMOLINK_ALWAYS_TESTS})
    add_test(NAME ${t} COMMAND ${t})
endforeach()
