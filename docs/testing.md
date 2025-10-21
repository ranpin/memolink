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
1. 数据采集模块捕获数据
2. 数据处理模块处理数据
3. 存储管理器存储数据
4. 记忆系统管理数据

#### 3.2.2 并发测试
测试多线程环境下的系统稳定性：
- 多模块同时运行
- 数据库并发访问
- 资源竞争处理

### 3.3 系统测试

#### 3.3.1 功能测试
测试系统所有功能的正确性：
- 用户管理
- 会话管理
- 数据采集
- 数据存储
- 数据检索
- AI模型集成

#### 3.3.2 性能测试
测试系统性能指标：
- 响应时间
- 吞吐量
- 资源使用率
- 并发处理能力

#### 3.3.3 稳定性测试
测试系统长时间运行的稳定性：
- 内存泄漏检测
- 资源释放验证
- 错误恢复能力

## 4. 演示程序测试

### 4.1 多模态演示 (multimodal_demo.cpp)
演示基本的多模态数据处理功能：
- 摄像头图像捕获
- 音频录制
- 屏幕截图
- 数据存储和检索

### 4.2 简单多模态演示 (simple_multimodal_demo.cpp)
演示简化的多模态处理流程：
- 集成化的多模态处理
- 简化的用户接口

### 4.3 真实多模态演示 (real_multimodal_demo.cpp)
演示完整的多模态处理功能：
- 真实的摄像头捕获
- 真实的音频录制
- 真实的屏幕截图
- 持续数据捕获
- 完整的用户交互

## 5. 测试执行

### 5.1 构建测试程序
```bash
cd build
cmake ..
make
```

### 5.2 运行测试
```bash
# 运行所有测试程序
./test_memory_managers
./test_agents
./test_multimodal
./test_long_term_memory
./test_content_metadata

# 运行演示程序
./multimodal_demo
./simple_multimodal_demo
./real_multimodal_demo
```

### 5.3 测试结果验证
- 检查程序输出日志
- 验证数据库中数据的正确性
- 检查生成的文件
- 验证系统资源使用情况

## 6. 自动化测试

### 6.1 持续集成
- 代码提交后自动运行测试
- 测试结果自动报告
- 失败情况自动通知

### 6.2 测试覆盖率
- 代码覆盖率统计
- 关键路径覆盖验证
- 边界条件测试

## 7. 测试报告

### 7.1 报告内容
- 测试执行情况汇总
- 通过/失败用例统计
- 性能指标
- 问题列表
- 改进建议

### 7.2 报告格式
- 文本报告
- HTML报告
- 图表展示

## 8. 测试维护

### 8.1 测试用例更新
- 随着功能增加更新测试用例
- 根据bug修复更新测试用例
- 定期审查测试用例有效性

### 8.2 测试环境维护
- 定期更新测试环境
- 维护测试数据
- 更新测试工具链

## 9. 常见问题及解决方案

### 9.1 摄像头无法访问
- 检查摄像头连接
- 验证权限设置
- 确认驱动程序

### 9.2 音频设备问题
- 检查音频设备连接
- 验证音频设置
- 确认PortAudio配置

### 9.3 数据库操作失败
- 检查数据库文件权限
- 验证SQL语句正确性
- 确认数据库连接

### 9.4 内存不足
- 检查系统内存使用
- 优化资源管理
- 增加虚拟内存