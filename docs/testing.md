# MIRIX系统测试文档

## 1. 概述

MIRIX系统包含全面的测试套件，用于验证系统各模块的功能正确性和稳定性。测试分为单元测试、集成测试和系统测试三个层次。

## 2. 测试环境

### 2.1 硬件环境
- 处理器：Intel/AMD 64位处理器
- 内存：至少8GB RAM
- 存储：至少100MB可用磁盘空间
- 摄像头：兼容的USB摄像头（可选）
- 音频设备：麦克风和扬声器（可选）

### 2.2 软件环境
- 操作系统：macOS 10.15+ / Ubuntu 18.04+ / Windows 10+
- 编译器：GCC 7.0+ / Clang 6.0+ / MSVC 2017+
- 构建工具：CMake 3.10+
- 依赖库：
  - OpenCV 4.0+
  - PortAudio
  - SQLite3
  - ScreenCaptureKit (macOS)

## 3. 测试套件

### 3.1 单元测试

#### 3.1.1 记忆管理器测试 (test_memory_managers.cpp)
测试所有记忆管理器的基本功能：
- CoreMemoryBlockManager
- EpisodicMemoryManager
- SemanticMemoryManager
- ProceduralMemoryManager
- ResourceMemoryManager
- KnowledgeVaultManager
- ArchivalMemoryManager
- RecallMemoryManager

测试内容：
- 添加记忆
- 更新记忆
- 删除记忆
- 搜索记忆
- 数据库操作

#### 3.1.2 代理测试 (test_agents.cpp)
测试代理相关功能：
- AgentManager
- CoreMemoryAgent
- EpisodicMemoryAgent
- SemanticMemoryAgent
- ProceduralMemoryAgent
- ResourceMemoryAgent
- KnowledgeVaultAgent
- ArchivalMemoryAgent

测试内容：
- 代理创建
- 代理功能调用
- 代理与记忆系统的交互

#### 3.1.3 多模态测试 (test_multimodal.cpp)
测试多模态数据处理功能：
- 摄像头捕获
- 音频录制
- 屏幕截图
- 多模态数据存储

测试内容：
- 单次数据捕获
- 持续数据捕获
- 数据存储验证
- 文件系统操作

#### 3.1.4 长期记忆测试 (test_long_term_memory.cpp)
测试长期记忆功能：
- 记忆持久化
- 记忆检索
- 记忆更新

测试内容：
- 长时间运行测试
- 大量数据存储测试
- 数据一致性验证

#### 3.1.5 内容元数据测试 (test_content_metadata.cpp)
测试内容元数据处理：
- 元数据生成
- 元数据存储
- 元数据检索

测试内容：
- 不同类型内容的元数据处理
- 元数据一致性验证

### 3.2 集成测试

#### 3.2.1 数据流测试
测试数据从采集到存储的完整流程：

#### 3.2.2 跨会话历史查询测试
测试跨会话历史记录查询功能：

##### 测试目标
- 验证用户在不同会话间的历史记录能够正确保存
- 验证能够查询用户最近几次会话的历史记录
- 验证会话间的数据隔离性和关联性

##### 测试步骤
1. 创建用户和默认Agent
2. 创建第一个会话
3. 进行若干次对话交互，生成回忆记忆
4. 结束第一个会话
5. 创建第二个会话
6. 进行若干次对话交互
7. 验证能够查询到第一个会话的历史记录
8. 验证查询结果按时间倒序排列

##### 测试代码示例
```cpp
// test_chat_with_model.cpp 中的测试逻辑
void test_cross_session_history() {
    // 初始化系统
    StorageManager storage("test.db");
    RecallMemoryManager recall_manager(storage);
    
    // 创建用户和会话
    int user_id = 1;
    
    // 会话1：添加一些记忆
    int session1_id = 1;
    recall_manager.add_recall_memory(user_id, 1, session1_id, "user_message", "你好");
    recall_manager.add_recall_memory(user_id, 1, session1_id, "ai_response", "你好！有什么可以帮助你的吗？");
    
    // 会话2：添加更多记忆
    int session2_id = 2;
    recall_manager.add_recall_memory(user_id, 1, session2_id, "user_message", "今天天气怎么样？");
    recall_manager.add_recall_memory(user_id, 1, session2_id, "ai_response", "今天天气很好！");
    
    // 测试跨会话查询
    auto memories = recall_manager.list_recent_sessions_memories(user_id, 5, 10);
    
    // 验证结果
    assert(memories.size() == 4); // 应该返回4条记录
    // 验证记录按时间倒序排列
    assert(memories[0].content == "今天天气很好！");
}
```

#### 3.2.3 多模态检索测试
测试多模态数据的检索功能：

##### 测试目标
- 验证文本和图像的向量化处理
- 验证跨模态相似性搜索
- 验证混合检索功能

##### 测试步骤
1. 添加文本和图像资源
2. 生成对应的向量表示
3. 执行文本到文本搜索
4. 执行图像到图像搜索
5. 执行跨模态搜索
6. 验证搜索结果的准确性

## 4. 系统测试

### 4.1 交互式测试 (test_chat_with_model.cpp)

#### 4.1.1 测试功能
- 用户对话交互
- 上下文构建
- 跨会话历史记录查询
- 向量混合搜索
- 多模态数据处理

#### 4.1.2 测试流程
1. 系统初始化
2. 用户和Agent管理
3. 会话创建
4. 用户输入处理
5. 上下文构建（包含历史记录和相关记忆）
6. 模型调用
7. 结果存储
8. 会话结束

#### 4.1.3 上下文构建测试
上下文构建是系统的核心功能之一，包含三个阶段：

##### 阶段1：获取历史对话信息
- 调用`RecallMemoryManager::list_recent_sessions_memories`获取最近几次会话的历史记录
- 默认查询用户最近5次会话中的最多10条消息
- 验证返回的历史记录按时间倒序排列

##### 阶段2：执行向量混合搜索
- 文本向量化：使用CLIP模型生成查询文本的embedding
- 相似性搜索：在回忆记忆和资源记忆中搜索相似内容
- 结果处理：对搜索结果进行归一化和排序

##### 阶段3：获取图片资源信息
- 查询与当前会话相关的图片资源
- 验证资源信息的完整性和准确性

#### 4.1.4 跨会话历史查询测试
- 验证系统能够正确获取用户最近的会话ID列表
- 验证能够查询这些会话中的回忆记忆
- 验证查询结果的正确性和完整性

#### 4.1.5 API调用测试
- 验证本地API调用功能
- 验证云端API调用功能
- 验证模拟响应功能
- 验证API调用优先级：本地API > 云端API > 模拟响应

### 4.2 多模态演示测试 (real_multimodal_demo.cpp)

#### 4.2.1 测试功能
- 摄像头图像捕获
- 音频录制
- 屏幕截图
- 多模态数据存储
- 多模态数据检索

#### 4.2.2 测试流程
1. 系统初始化
2. 多模态数据捕获
3. 数据存储到记忆系统
4. 向量化处理
5. 数据检索验证

## 5. 性能测试

### 5.1 数据库性能测试
- 大量数据插入性能
- 查询性能测试
- 索引效果验证

### 5.2 向量检索性能测试
- 向量生成性能
- 相似性搜索性能
- 混合检索性能

### 5.3 系统响应时间测试
- 用户输入响应时间
- 模型调用延迟
- 结果返回时间

## 6. 回归测试

### 6.1 数据持久化测试
- 验证系统重启后数据不丢失
- 验证跨会话数据访问正确性
- 验证数据库完整性

### 6.2 功能完整性测试
- 验证所有核心功能正常工作
- 验证各模块间协作正常
- 验证异常处理机制

## 7. 测试报告

### 7.1 测试覆盖率
- 代码覆盖率统计
- 功能覆盖率统计
- 边界条件测试

### 7.2 性能指标
- 平均响应时间
- 吞吐量
- 资源使用情况

### 7.3 问题跟踪
- 已知问题列表
- 问题修复验证
- 回归问题检查

## 8. 持续集成测试

### 8.1 自动化测试
- 构建触发测试
- 单元测试自动化执行
- 集成测试自动化执行

### 8.2 测试环境管理
- 测试环境自动部署
- 测试数据管理
- 测试结果收集和分析