# MIRIX - 多模态集成推理系统

MIRIX是一个创新的多模态记忆系统，旨在模仿人类记忆机制，通过整合文本、图像、音频等多种感官输入，构建丰富的情景记忆体验。

## 功能特点

- 🧠 高级记忆系统（情景记忆、语义记忆、程序记忆等）
- 👁️ 多模态数据处理（文本、图像、音频、视频）
- 🤖 智能代理系统
- 📊 向量数据库和相似性搜索
- 🎙️ 实时音频捕获和处理
- 📷 视频捕获和分析
- 🌐 API服务接口
- 🖥️ 图形用户界面（基于Qt5）

## 系统架构

请参阅 [架构文档](docs/architecture.md) 了解详细信息。

```
graph TD
    A[用户接口层] --> B[模型与AI管理层]
    B --> C[记忆处理层]
    C --> D[存储管理层]
    D --> E[数据库层]
    
    F[CLIP模型] --> B
    G[文本嵌入模型] --> B
    H[多模态数据处理] --> C
    I[记忆管理器] --> C
    J[SQLite数据库] --> E
    
    style A fill:#ffe4c4,stroke:#333
    style B fill:#d8bfd8,stroke:#333
    style C fill:#98fb98,stroke:#333
    style D fill:#87ceeb,stroke:#333
    style E fill:#ffa07a,stroke:#333
    style F fill:#dda0dd,stroke:#333
    style G fill:#dda0dd,stroke:#333
    style H fill:#98fb98,stroke:#333
    style I fill:#98fb98,stroke:#333
    style J fill:#ffa07a,stroke:#333
```

## 跨平台支持

MIRIX支持以下操作系统：

- macOS (推荐使用Homebrew进行依赖管理)
- Linux (支持Ubuntu/Debian, CentOS/RHEL, Arch Linux等发行版)

Windows支持正在开发中。

## 依赖库管理策略

为了提高项目的可移植性和易用性，MIRIX采用以下依赖库管理策略：

### 1. 本地依赖库结构
MIRIX使用纯本地依赖库，所有依赖都包含在项目中，按照平台分别存储在以下目录中：
- macOS依赖: `libs/macos/`
- Linux依赖: `libs/linux/`

### 2. 依赖库列表
MIRIX使用以下核心依赖库：
- SQLite3 - 数据库存储
- OpenSSL - 安全通信
- cURL - HTTP请求
- JSONCPP - JSON处理
- PortAudio - 音频处理
- Faiss - 向量相似性搜索
- OpenCV - 计算机视觉
- Qt5 (可选) - 图形用户界面
- sqlite_orm - C++ ORM库

### 3. 依赖库管理方法
虽然项目中包含了本地依赖库，但某些系统级依赖（如图形框架、系统库等）仍需要在目标系统上安装。

对于Linux系统，我们提供了一个脚本 [scripts/prepare_linux_libs.sh](scripts/prepare_linux_libs.sh) 来说明如何准备库文件。

有关详细信息，请参阅 [依赖管理文档](docs/dependency_management.md)。

## ORM重构

为了提高代码质量和维护性，我们对数据库访问层进行了重构，采用了基于sqlite_orm库的对象关系映射(ORM)架构。这一重构带来了以下优势：

- 字段解耦：所有字段定义集中在模型结构体中
- 类型安全：编译时检查字段和类型
- 减少样板代码：ORM自动生成CRUD操作
- 统一查询接口：提供一致的查询方式
- 向后兼容：保持现有接口不变
- 简洁接口命名：新接口命名直观简洁

详细信息请参阅 [ORM重构文档](docs/ORM重构文档.md)。

## 构建项目

1. 创建构建目录：
   ```bash
   mkdir build && cd build
   ```

2. 运行CMake：
   ```bash
   cmake ..
   ```

3. 编译项目：
   ```bash
   make
   ```

## 项目结构

```
mirix-cpp/
├── CMakeLists.txt              # CMake构建配置文件
├── README.md                   # 项目说明文档
├── DETAILS.md                  # 详细设计文档
├── captures/                   # 多模态数据捕获目录
│   └── images/                 # 图像捕获存储目录
├── database/                   # 数据库存储目录
│   └── mirix.db               # 默认SQLite数据库文件
├── docs/                       # 项目文档目录
│   ├── architecture.md         # 系统架构文档
│   ├── database.md             # 数据库设计文档
│   ├── memory.md               # 记忆系统文档
│   ├── multimodal.md           # 多模态处理文档
│   ├── testing.md              # 测试文档
│   └── improvements.md         # 改进建议文档
├── src/                        # 源代码目录
│   ├── agents/                 # 智能代理模块
│   ├── capture/                # 多模态数据捕获模块
│   ├── database/               # 数据库相关模块
│   │   ├── core/              # 数据库核心模块
│   │   ├── initialization/    # 数据库初始化模块
│   │   └── managers/          # 数据库管理器模块
│   ├── models/                 # AI模型接口模块
│   └── utils/                 # 工具类模块
└── test/                       # 测试代码目录
    ├── CMakeLists.txt          # 测试构建配置
    └── test_*.cpp              # 各类测试程序
```

## 运行测试

构建完成后，您可以运行测试来验证系统功能：

```bash
./test_memory_generation
./test_context_builder
# 运行其他测试...
```

## 文档

- [架构文档](docs/architecture.md)
- [数据库设计](docs/database.md)
- [多模态处理](docs/multimodal.md)
- [记忆系统](docs/memory.md)
- [测试策略](docs/testing.md)
- [改进计划](docs/improvements.md)
- [依赖管理](docs/dependency_management.md)
- [ORM重构文档](docs/ORM重构文档.md)

## 许可证

本项目采用MIT许可证 - 请查看 [LICENSE](LICENSE) 文件了解详情。