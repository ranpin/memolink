cmake_minimum_required(VERSION 3.10)
project(Mirix VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 查找所需的包
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
find_path(CURL_INCLUDE_DIRS curl/curl.h)

# 在macOS上查找CoreGraphics和ApplicationServices框架
if(APPLE)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics)
    find_library(APP_SERVICES_FRAMEWORK ApplicationServices)
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/libs/macos/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/include/sqlite_orm/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# 库目录
link_directories(${CMAKE_SOURCE_DIR}/libs/macos/lib)

# 收集所有源文件
file(GLOB_RECURSE SOURCES "src/**/*.cpp")

# 排除测试文件和主文件
list(FILTER SOURCES EXCLUDE REGEX "test_.*\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*/test/.*\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "simple_orm_test\\.cpp$")

# 添加主程序可执行文件
add_executable(mirix src/main.cpp ${SOURCES})

# 链接库
target_link_libraries(mirix PRIVATE 
    sqlite3 
    faiss
    opencv_core 
    opencv_imgproc 
    opencv_highgui 
    opencv_videoio 
    opencv_imgcodecs
    ${CURL_LIBRARIES}
    jsoncpp
    portaudio
)

# 在macOS上链接额外的框架
if(APPLE)
    target_link_libraries(mirix PRIVATE 
        ${CORE_GRAPHICS_FRAMEWORK} 
        ${APP_SERVICES_FRAMEWORK}
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudio"
    )
endif()

# 创建静态库
add_library(agents STATIC src/agents/ChatAgent.cpp src/agents/MemoryAgent.cpp)
add_library(capture STATIC 
    src/capture/AudioCaptureManager.cpp 
    src/capture/CameraCaptureManager.cpp 
    src/capture/MultimodalDataManager.cpp 
    src/capture/ScreenActivityTracker.cpp)
add_library(database STATIC 
    src/database/core/ORMWrapper.cpp 
    src/database/core/StorageManager.cpp 
    src/database/core/TransactionManager.cpp 
    src/database/initialization/ConfigManager.cpp 
    src/database/managers/CoreMemoryBlockManager.cpp 
    src/database/managers/EpisodicMemoryManager.cpp 
    src/database/managers/MemoryOrchestrator.cpp 
    src/database/managers/MemoryOrchestratorImpl.cpp 
    src/database/managers/MessageMemoryManager.cpp 
    src/database/managers/ProceduralMemoryManager.cpp 
    src/database/managers/SecretVaultManager.cpp 
    src/database/managers/SemanticMemoryManager.cpp 
    src/database/managers/SessionManager.cpp 
    src/database/managers/UserManager.cpp)
add_library(memory STATIC 
    src/memory/context/ContextBuilder.cpp 
    src/memory/generation/MemoryClassifier.cpp 
    src/memory/generation/MemoryGenerationManager.cpp 
    src/memory/generation/generators/BaseMemoryGenerator.cpp
    src/memory/generation/generators/CoreMemoryGenerator.cpp
    src/memory/generation/generators/EpisodicMemoryGenerator.cpp
    src/memory/generation/generators/ProceduralMemoryGenerator.cpp
    src/memory/generation/generators/SecretVaultGenerator.cpp
    src/memory/generation/generators/SemanticMemoryGenerator.cpp
    src/memory/messages/MessageManager.cpp
    src/memory/scheduling/MemoryGenerationScheduler.cpp)
add_library(models STATIC 
    src/models/CLIPProcessor.cpp
    src/models/EmbeddingIndexManager.cpp
    src/models/ModelManager.cpp
    src/models/PromptTemplates.cpp
    src/models/QwenAPI.cpp
    src/models/TextEmbeddingGenerator.cpp
    src/models/VectorSearchManager.cpp)
add_library(utils STATIC 
    src/utils/TimeUtils.cpp
    src/utils/VectorSerializer.cpp)
add_library(cli STATIC src/cli/CommandLineInterface.cpp)

# 链接静态库到主程序
target_link_libraries(mirix PRIVATE agents capture database memory models utils cli)

# 修复其他测试可执行文件的链接问题
target_link_libraries(database PRIVATE sqlite3 jsoncpp ${CURL_LIBRARIES})
target_link_libraries(models PRIVATE sqlite3 jsoncpp utils ${CURL_LIBRARIES})
target_link_libraries(memory PRIVATE sqlite3 jsoncpp utils ${CURL_LIBRARIES})
target_link_libraries(agents PRIVATE sqlite3 jsoncpp ${CURL_LIBRARIES})
target_link_libraries(cli PRIVATE sqlite3 jsoncpp ${CURL_LIBRARIES})
target_link_libraries(capture PRIVATE sqlite3 jsoncpp portaudio ${CURL_LIBRARIES})

# 添加测试可执行文件
add_executable(test_system_initialization test/test_system_initialization.cpp)
target_link_libraries(test_system_initialization PRIVATE database)

add_executable(test_memory_generation test/test_memory_generation.cpp)
target_link_libraries(test_memory_generation PRIVATE database models memory utils)

add_executable(test_context_builder test/test_context_builder.cpp)
target_link_libraries(test_context_builder PRIVATE database models memory utils)

add_executable(test_vector_serializer test/test_vector_serializer.cpp)
target_link_libraries(test_vector_serializer PRIVATE utils)

add_executable(test_real_multimodal_demo test/test_real_multimodal_demo.cpp src/utils/TimeUtils.cpp src/utils/VectorSerializer.cpp)
target_link_libraries(test_real_multimodal_demo PRIVATE database models capture memory agents
    opencv_core 
    opencv_imgproc 
    opencv_highgui 
    opencv_videoio 
    opencv_imgcodecs 
    ${CURL_LIBRARIES})

# macOS 下为多模态演示程序链接图形和音频框架
if(APPLE)
    target_link_libraries(test_real_multimodal_demo PRIVATE 
        ${CORE_GRAPHICS_FRAMEWORK} 
        ${APP_SERVICES_FRAMEWORK}
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudio"
    )
endif()