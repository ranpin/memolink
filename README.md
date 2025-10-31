# MIRIX - 多模态记忆系统

MIRIX是一个仿人类记忆系统的多模态数据处理框架，能够捕获、存储和管理图像、音频、屏幕截图等多种类型的数据，并将其组织成类似人类记忆的结构。

## 功能特性

### 多模态数据处理
- **图像捕获**：支持摄像头图像捕获和存储
- **音频录制**：支持系统音频录制和存储
- **屏幕截图**：支持屏幕内容捕获和存储
- **持续捕获**：支持定时自动捕获各类数据

### 仿人类记忆系统
系统实现了完整的人类记忆模型，包括：
- **核心记忆**：存储最基本和重要的信息块
- **事件记忆**：存储个人经历和事件相关信息
- **语义记忆**：存储概念性知识和事实信息
- **程序记忆**：存储技能和过程性知识
- **资源记忆**：存储多媒体资源文件的元信息
- **知识库**：存储结构化知识和信息
- **归档记忆**：存储长期保存的历史数据
- **回忆记忆**：存储记忆检索过程和结果

### AI模型集成
- **通义千问API**：集成Qwen大模型进行文本处理
- **向量检索**：支持基于向量的相似性搜索
- **多模态处理**：支持图像、音频和文本的联合处理
- **本地API支持**：支持OpenAI兼容的本地服务调用

### 数据管理
- **SQLite数据库**：使用SQLite进行数据持久化存储
- **事务管理**：支持数据库事务确保数据一致性
- **索引优化**：建立索引提高数据检索效率

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层                               │
│  多模态演示  简单演示  交互式主程序  初始化程序等           │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                    模型与AI管理层                          │
│  模型管理器  QwenAPI接口  嵌入索引管理器  CLIP处理器        │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                      记忆系统层                             │
│  核心记忆 事件记忆 语义记忆 程序记忆 资源记忆 知识库等      │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                      数据管理层                             │
│  多模态数据管理器 存储管理器 事务管理器                     │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                      数据采集层                             │
│  摄像头 音频 屏幕活动 系统事件                              │
└─────────────────────────────────────────────────────────────┘
```

## 项目结构

```
mirix-cpp/
├── CMakeLists.txt              # 构建配置文件
├── README.md                   # 项目说明文档
├── DETAILS.md                  # 详细设计文档
├── config.ini                  # 配置文件
├── init.sql                    # 数据库初始化脚本
├── docs/                       # 详细文档目录
│   ├── architecture.md         # 系统架构文档
│   ├── database.md             # 数据库设计文档
│   ├── memory.md               # 记忆系统文档
│   ├── multimodal.md           # 多模态处理文档
│   ├── testing.md              # 测试文档
│   └── improvements.md         # 改进建议文档
├── src/                        # 源代码目录
│   ├── agents/                 # 代理相关代码
│   ├── api/                    # API接口代码
│   ├── capture/                # 数据捕获代码
│   ├── cli/                    # 命令行界面代码
│   ├── database/               # 数据库相关代码
│   │   ├── core/               # 核心数据库功能
│   │   ├── initialization/     # 数据库初始化
│   │   └── managers/           # 数据库管理器
│   ├── gui/                    # 图形界面代码
│   ├── models/                 # AI模型相关代码
│   ├── storage/                # 存储管理代码
│   └── utils/                  # 工具类代码
├── test/                       # 测试代码目录
│   ├── test_embedding_functionality.cpp      # 基础嵌入功能测试
│   ├── test_embedding_comprehensive.cpp      # 综合嵌入功能测试
│   ├── test_vector_serializer.cpp            # 向量序列化测试
│   ├── test_clip_processor.cpp               # CLIP处理器测试
│   ├── test_chat_with_model.cpp              # 模型对话测试
│   ├── test_chat_with_auto_capture.cpp       # 自动捕获功能测试
│   ├── test_real_multimodal_demo.cpp         # 多模态演示测试
│   ├── test_initialize_memories.cpp          # 初始化记忆测试
│   └── test_system_initialization.cpp        # 系统初始化测试
├── database/                   # 数据库文件目录
├── captures/                   # 捕获数据存储目录
└── test_images/                # 测试图像目录
```

## 安装与构建

### 系统要求
- C++17 或更高版本
- CMake 3.10 或更高版本
- SQLite3
- OpenSSL
- cURL
- PortAudio
- OpenCV
- FAISS (Facebook AI Similarity Search)

### 构建步骤

1. 克隆项目仓库：
```bash
git clone <repository_url>
cd mirix-cpp
```

2. 创建构建目录并进入：
```bash
mkdir build
cd build
```

3. 运行CMake配置：
```bash
cmake ..
```

4. 编译项目：
```bash
make
```

## 使用方法

构建完成后，可以在build目录下找到以下可执行文件：

- `real_multimodal_demo` - 多模态演示程序
- `initialize_memories` - 初始化记忆系统
- `test_embedding` - 基础嵌入功能测试
- `test_vector_serializer` - 向量序列化测试
- `test_embedding_comprehensive` - 综合嵌入功能测试
- `test_clip_processor` - CLIP处理器测试
- `test_chat_with_model` - 模型对话测试
- `test_chat_with_auto_capture` - 自动捕获功能测试
- `test_initialize_memories` - 初始化记忆测试
- `test_system_initialization` - 系统初始化测试

运行任一程序：
```bash
./<executable_name>
```

## 配置说明

项目使用 `config.ini` 文件进行配置，主要配置项包括：

- API密钥设置
- 模型名称配置
- 数据库路径设置

确保在运行程序前正确配置该文件。

## 文档

详细的系统文档可以在 `docs/` 目录中找到：

- [架构文档](docs/architecture.md)
- [数据库设计](docs/database.md)
- [记忆系统](docs/memory.md)
- [多模态处理](docs/multimodal.md)
- [测试说明](docs/testing.md)
- [改进建议](docs/improvements.md)

## 测试

项目包含完整的测试套件，可以使用以下命令运行测试：

```bash
make test
```

或者单独运行某个测试程序：
```bash
./<test_executable_name>
```