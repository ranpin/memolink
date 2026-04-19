# 添加GoogleTest模块
find_package(GTest REQUIRED)

# 定义测试源文件
set(TEST_SOURCES
    test_all_orm_managers.cpp
    test_chat_with_auto_capture.cpp
    test_chat_with_model.cpp
    test_cleanup_orphan_embeddings.cpp
    test_clip_processor.cpp
    test_comprehensive_memory.cpp
    test_comprehensive_system.cpp
    test_context_builder.cpp
    test_db_debug.cpp
    test_embedding_comprehensive.cpp
    test_embedding_functionality.cpp
    test_index_consistency_check.cpp
    test_initialize_memories.cpp
    test_memory_generation.cpp
    test_orm_transaction.cpp
    test_real_multimodal_demo.cpp
    test_simple_orm_test.cpp
    test_sqlite_orm.cpp
    test_system_initialization.cpp
    test_vector_serializer.cpp
    test_transaction.cpp
    test_batch_embedding_generation.cpp  # 批量生成embedding工具
)

# 为每个测试源文件创建可执行文件
foreach(test_source ${TEST_SOURCES})
    # 获取不带路径和扩展名的文件名作为目标名
    get_filename_component(target_name ${test_source} NAME_WE)
    
    # 创建可执行文件
    add_executable(${target_name} ${test_source})
    
    # 链接必要的库
    target_link_libraries(${target_name} 
        PRIVATE 
            GTest::Main
            mirix_agents
            mirix_database
            mirix_memory
            mirix_models
            mirix_utils
            SQLite::SQLite3
    )
    
    # 设置包含目录
    target_include_directories(${target_name} 
        PRIVATE 
            ${CMAKE_SOURCE_DIR}/libs/include
            ${CMAKE_SOURCE_DIR}/libs/include/sqlite_orm/include
    )
endforeach()

# 特殊处理需要额外依赖的测试
target_link_libraries(test_clip_processor PRIVATE ${OpenCV_LIBS})
target_link_libraries(test_real_multimodal_demo PRIVATE ${OpenCV_LIBS})