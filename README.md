# MIRIX - 多模态交互记忆系统

## 项目概述

MIRIX是一个先进的多模态交互记忆系统，旨在模拟人类记忆机制，实现对文本、图像、音频等多种模态信息的存储、检索和关联。该系统采用C++开发，结合现代AI技术，为用户提供智能记忆管理服务。

## 系统架构

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

## 项目结构

```
mirix-cpp/
├── CMakeLists.txt              # CMake构建配置文件
├── README.md                   # 项目说明文档
├── DETAILS.md                  # 详细设计文档
├── config.ini                  # 运行配置文件（API/模型/数据库）
├── captures/                   # 多模态数据捕获目录（运行时生成，不入库）
│   ├── camera/                 # 摄像头图像捕获存储目录
│   └── screen/                 # 屏幕截图捕获存储目录
├── database/                   # 数据库存储目录
│   └── mirix.db               # 默认SQLite数据库文件
├── docs/                       # 项目文档目录
│   ├── architecture.md         # 系统架构文档
│   ├── database.md             # 数据库设计文档
│   ├── memory.md               # 记忆系统文档
│   ├── multimodal.md           # 多模态处理文档
│   ├── testing.md              # 测试文档
│   └── improvements.md         # 改进建议文档
├── models/                     # 模型文件目录（CLIP配置、ONNX模型）
├── scripts/                    # 辅助脚本（CLIP处理、模型下载）
├── src/                        # 源代码目录
│   ├── agents/                 # 智能代理模块
│   ├── api/                    # API服务模块
│   ├── capture/                # 多模态数据捕获模块
│   ├── cli/                    # 命令行交互模块
│   ├── database/               # 数据库相关模块
│   │   ├── core/              # 数据库核心模块
│   │   ├── initialization/    # 数据库初始化模块
│   │   └── managers/          # 数据库管理器模块
│   ├── gui/                    # GUI模块
│   ├── models/                 # AI模型接口模块
│   ├── storage/                # 存储模块
│   └── utils/                 # 工具类模块
├── test_images/                # 测试用图像素材
└── test/                       # 测试代码目录
    └── test_*.cpp              # 各类测试程序
```


## 安装与构建

### 系统要求
- C++17 或更高版本
- CMake 3.10 或更高版本
- SQLite3
- OpenSSL
- cURL
- JsonCpp
- PortAudio
- OpenCV
- FAISS (Facebook AI Similarity Search)
- Qt5（可选，仅在需要构建 GUI 目标时）

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