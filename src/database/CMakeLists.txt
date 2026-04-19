# 定义数据库模块的源文件
set(DATABASE_SOURCES
    core/ORMWrapper.cpp
    core/StorageManager.cpp
    core/TransactionManager.cpp
    core/MemoryManager.cpp
    initialization/ConfigManager.cpp
    managers/CoreMemoryBlockManager.cpp
    managers/EpisodicMemoryManager.cpp
    managers/MessageMemoryManager.cpp
    managers/ProceduralMemoryManager.cpp
    managers/ResourceMemoryManager.cpp
    managers/SecretVaultManager.cpp
    managers/SemanticMemoryManager.cpp
    managers/SessionManager.cpp
    managers/UserManager.cpp
    managers/MemoryOrchestrator.cpp
    managers/MemoryOrchestratorImpl.cpp
    managers/MemoryProvenanceManager.cpp
)

# 定义数据库模块的头文件
set(DATABASE_HEADERS
    core/ORMWrapper.h
    core/StorageManager.h
    core/TransactionManager.h
    core/MemoryManager.h
    core/MemoryManagerInterface.h
    initialization/ConfigManager.h
    managers/CoreMemoryBlockManager.h
    managers/EpisodicMemoryManager.h
    managers/MessageMemoryManager.h
    managers/ProceduralMemoryManager.h
    managers/ResourceMemoryManager.h
    managers/SecretVaultManager.h
    managers/SemanticMemoryManager.h
    managers/SessionManager.h
    managers/UserManager.h
    managers/MemoryOrchestrator.h
    managers/MemoryOrchestratorImpl.h
    models/MemoryModels.h
    managers/MemoryProvenanceManager.h
)

# 创建静态库
add_library(mirix_database STATIC ${DATABASE_SOURCES} ${DATABASE_HEADERS})

# 链接 SQLite3 库
target_link_libraries(mirix_database PRIVATE SQLite::SQLite3)

# 设置包含目录
target_include_directories(mirix_database 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/include/sqlite_orm/include
)