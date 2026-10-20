cmake_minimum_required(VERSION 3.10)
project(MIRIX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖库
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Qt5 COMPONENTS Widgets Core Gui OPTIONAL_COMPONENTS)

# 查找SQLite3
find_package(SQLite3 REQUIRED)

# 查找cURL
find_package(CURL REQUIRED)

# 查找JsonCpp
find_package(jsoncpp REQUIRED)
if(NOT jsoncpp_FOUND)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

# 查找PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# 查找OpenCV
find_package(OpenCV REQUIRED)

# 注：向量检索目前是对 SQLite 中存储的 embedding 做暴力余弦相似度
# （见 src/models/VectorSearchManager.cpp），并未链接 FAISS。
# EmbeddingIndexManager 里的 FAISS 方法仍是占位符，待后续接入；
# 因此这里不再 find_package(faiss REQUIRED)——否则没装 FAISS 连 configure 都过不了。

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})

# 链接目录
link_directories(${CURL_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})
link_directories(${PORTAUDIO_LIBRARY_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})

# 收集源文件（明确列出所有源文件）
set(SOURCES
    # 核心源文件
    src/database/core/StorageManager.cpp
    src/database/core/TransactionManager.cpp
    src/database/initialization/InitDB.cpp
    src/database/initialization/ConfigManager/ConfigManager.cpp
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
    src/models/ModelManager.cpp
    src/models/QwenAPI.cpp
    src/models/TextEmbeddingGenerator.cpp
    src/models/VectorSearchManager.cpp
    src/models/EmbeddingIndexManager.cpp
    src/models/CLIPProcessor.cpp
    src/agents/AgentManager.cpp
    src/agents/AgentWrapper.cpp
    src/agents/ChatAgent.cpp
    src/agents/CoreMemoryAgent.cpp
    src/agents/EpisodicMemoryAgent.cpp
    src/agents/SemanticMemoryAgent.cpp
    src/agents/ProceduralMemoryAgent.cpp
    src/agents/ResourceMemoryAgent.cpp
    src/agents/KnowledgeVaultAgent.cpp
    src/capture/CameraCaptureManager.cpp
    src/capture/AudioCaptureManager.cpp
    src/capture/ScreenActivityTracker.cpp
    src/capture/SystemEventManager.cpp
    src/capture/MultimodalDataManager.cpp
    src/utils/VectorSerializer.cpp
    src/utils/TimeUtils.cpp
    src/cli/CommandLineInterface.cpp
    src/api/APIServer.cpp
    src/gui/GUIManager.cpp
    src/storage/StorageManager.cpp
    
    # 测试程序
    test/test_embedding_functionality.cpp
    test/test_vector_serializer.cpp
    test/test_embedding_comprehensive.cpp
    test/test_clip_processor.cpp
    test/test_chat_with_model.cpp
    test/test_chat_with_auto_capture.cpp
    test/test_system_initialization.cpp
)

# 移除重复的测试源文件
list(REMOVE_ITEM SOURCES 
    src/test_comprehensive.cpp
    src/test_multimodal_integration.cpp
    src/test_real_embedding.cpp
    src/test_real_model_integration.cpp
    src/test_system_comprehensive.cpp
    src/test_text_embedding_generator.cpp
    src/test_vector_search_manager.cpp
    src/test_vector_search_real_model.cpp
)

# 处理Objective-C++源文件（如果存在）
if(APPLE)
    # 查找Objective-C++源文件
    file(GLOB_RECURSE OBJCPP_SOURCES "src/*.mm")
    if(OBJCPP_SOURCES)
        list(APPEND SOURCES ${OBJCPP_SOURCES})
    endif()
endif()

# 创建静态库
add_library(mirix_lib STATIC ${SOURCES})
target_link_libraries(mirix_lib 
    SQLite::SQLite3
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    jsoncpp  # 添加这一行以确保正确链接JsonCPP库
    ${PORTAUDIO_LIBRARIES}
    ${OpenCV_LIBS}
    ${FAISS_LIBRARIES}
)

if(APPLE)
    if(TARGET mirix_lib AND OBJCPP_SOURCES)
        set_source_files_properties(${OBJCPP_SOURCES} PROPERTIES LANGUAGE CXX)
    endif()
    
    # 为使用屏幕捕获功能的可执行文件链接ApplicationServices框架
    target_link_libraries(mirix_lib "-framework ApplicationServices")
endif()

# 创建可执行文件并链接

add_executable(real_multimodal_demo test/test_real_multimodal_demo.cpp)
target_link_libraries(real_multimodal_demo mirix_lib)

add_executable(initialize_memories test/test_initialize_memories.cpp)
target_link_libraries(initialize_memories mirix_lib)

add_executable(test_embedding test/test_embedding_functionality.cpp)
target_link_libraries(test_embedding mirix_lib)

add_executable(test_vector_serializer test/test_vector_serializer.cpp)
target_link_libraries(test_vector_serializer mirix_lib)

add_executable(test_embedding_comprehensive test/test_embedding_comprehensive.cpp)
target_link_libraries(test_embedding_comprehensive mirix_lib)

add_executable(test_clip_processor test/test_clip_processor.cpp)
target_link_libraries(test_clip_processor mirix_lib)

add_executable(test_chat_with_model test/test_chat_with_model.cpp)
target_link_libraries(test_chat_with_model mirix_lib)

add_executable(test_chat_with_auto_capture test/test_chat_with_auto_capture.cpp)
target_link_libraries(test_chat_with_auto_capture mirix_lib)

add_executable(test_initialize_memories test/test_initialize_memories.cpp)
target_link_libraries(test_initialize_memories mirix_lib)

# 添加新的测试可执行文件
add_executable(test_system_initialization test/test_system_initialization.cpp)
target_link_libraries(test_system_initialization mirix_lib)

# GUI 可执行文件
# 注：GUI 入口 src/gui_main.cpp 与 MacOSXBundleInfo.plist.in 目前并不存在
# （src/gui/ 下只有 GUIManager.cpp/.h），若 Qt5 被找到，这段会让 configure 直接失败。
# 先注释掉，等补上 gui_main.cpp 再启用。
# if(Qt5_FOUND)
#     add_executable(mirix_gui WIN32 src/gui_main.cpp)
#     target_link_libraries(mirix_gui mirix_lib Qt5::Widgets Qt5::Core Qt5::Gui)
#     if(APPLE)
#         set_target_properties(mirix_gui PROPERTIES
#             MACOSX_BUNDLE TRUE
#             MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/src/gui/MacOSXBundleInfo.plist.in
#         )
#     endif()
# endif()

# 安装规则
install(TARGETS real_multimodal_demo initialize_memories test_embedding test_vector_serializer test_embedding_comprehensive test_clip_processor test_chat_with_model test_chat_with_auto_capture test_initialize_memories test_system_initialization
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 添加测试
enable_testing()

add_test(NAME test_embedding_comprehensive COMMAND test_embedding_comprehensive)
add_test(NAME test_embedding COMMAND test_embedding)
add_test(NAME test_vector_serializer COMMAND test_vector_serializer)
add_test(NAME test_clip_processor COMMAND test_clip_processor)
add_test(NAME test_chat_with_model COMMAND test_chat_with_model)
add_test(NAME test_chat_with_auto_capture COMMAND test_chat_with_auto_capture)
add_test(NAME test_system_initialization COMMAND test_system_initialization)
add_test(NAME test_initialize_memories COMMAND test_initialize_memories)
add_test(NAME test_real_multimodal_demo COMMAND test_real_multimodal_demo)


# 安装头文件
install(DIRECTORY src/ 
    DESTINATION include/mirix
    FILES_MATCHING PATTERN "*.h"
)

# 安装配置文件
install(FILES config.ini DESTINATION etc)