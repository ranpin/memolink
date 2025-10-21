# MIRIX多模态数据处理文档

## 1. 概述

MIRIX系统支持处理多种类型的多模态数据，包括图像、音频和屏幕截图。系统通过专门的模块来捕获、处理和存储这些数据，并将其集成到记忆系统中。

## 2. 数据类型支持

### 2.1 图像数据
- 来源：摄像头捕获
- 格式：JPEG
- 存储路径：captures/camera/
- 记忆类型：resource_memory表中的image类型

### 2.2 音频数据
- 来源：系统音频录制
- 格式：WAV
- 存储路径：captures/audio/
- 记忆类型：resource_memory表中的audio类型

### 2.3 屏幕截图数据
- 来源：屏幕捕获
- 格式：PNG
- 存储路径：captures/screen/
- 记忆类型：resource_memory表中的screenshot类型

## 3. 数据采集模块

### 3.1 摄像头捕获模块 (CameraCaptureManager)

#### 3.1.1 功能特性
- 支持单次图像捕获
- 支持持续图像捕获
- 使用OpenCV库实现
- 支持JPEG格式存储

#### 3.1.2 核心方法
- capture_image(): 捕获单张图像
- start_continuous_capture(): 开始持续捕获
- stop_continuous_capture(): 停止持续捕获
- store_capture_in_memory(): 存储到记忆系统

#### 3.1.3 配置参数
- 摄像头设备ID（默认为0）
- 图像分辨率（默认1280x720）
- 捕获间隔时间（秒）

### 3.2 音频捕获模块 (AudioCaptureManager)

#### 3.2.1 功能特性
- 支持单次音频录制
- 支持持续音频录制
- 使用PortAudio库实现
- 支持WAV格式存储

#### 3.2.2 核心方法
- start_audio_capture(): 开始音频录制
- stop_audio_capture(): 停止音频录制
- start_continuous_audio_capture(): 开始持续录制
- stop_continuous_audio_capture(): 停止持续录制
- store_audio_in_memory(): 存储到记忆系统

#### 3.2.3 配置参数
- 录制时长（秒）
- 录制间隔时间（秒）
- 采样率（默认44100Hz）
- 声道数（默认单声道）

### 3.3 屏幕活动跟踪模块 (ScreenActivityTracker)

#### 3.3.1 功能特性
- 支持单次屏幕截图
- 支持持续屏幕截图
- 使用ScreenCaptureKit和系统命令实现
- 支持PNG格式存储

#### 3.3.2 核心方法
- capture_screenshot(): 捕获屏幕截图
- start_continuous_screenshot(): 开始持续截图
- stop_continuous_screenshot(): 停止持续截图
- store_screenshot_in_memory(): 存储到记忆系统

#### 3.3.3 配置参数
- 截图间隔时间（秒）
- 截图质量参数

## 4. 数据处理流程

### 4.1 单次捕获流程
1. 用户触发捕获命令
2. 相应模块执行数据捕获
3. 数据保存到文件系统
4. 元数据存储到resource_memory表
5. 返回捕获结果给用户

### 4.2 持续捕获流程
1. 用户启动持续捕获
2. 系统启动后台线程
3. 按设定间隔自动捕获数据
4. 数据保存到文件系统
5. 元数据存储到resource_memory表
6. 用户可随时停止持续捕获

## 5. 数据存储

### 5.1 文件系统存储
所有多模态数据都以文件形式存储在文件系统中，按类型分类存储：
- 图像：captures/camera/
- 音频：captures/audio/
- 屏幕截图：captures/screen/

### 5.2 数据库存储
数据的元信息存储在resource_memory表中，包括：
- 文件路径和名称
- 资源类型
- 文件大小
- 创建时间
- 关联的用户和会话信息

## 6. 数据检索

### 6.1 按类型检索
用户可以通过resource_type字段检索特定类型的资源：
- image：图像资源
- audio：音频资源
- screenshot：屏幕截图资源

### 6.2 按时间检索
用户可以通过created_at字段检索特定时间范围内的资源。

### 6.3 按用户检索
用户可以通过user_id字段检索特定用户的资源。

## 7. 持续捕获功能

### 7.1 功能描述
持续捕获功能允许系统按设定的时间间隔自动捕获多模态数据，无需用户干预。

### 7.2 配置参数
- 摄像头捕获间隔（默认30秒）
- 音频录制时长（默认5秒）
- 音频录制间隔（默认30秒）
- 屏幕截图间隔（默认30秒）

### 7.3 控制接口
- start_continuous_capture(): 启动所有类型的持续捕获
- stop_continuous_capture(): 停止所有类型的持续捕获
- start/stop特定类型捕获的方法

## 8. 性能优化

### 8.1 资源管理
- 使用智能指针管理资源
- 及时释放不需要的资源
- 多线程安全处理

### 8.2 存储优化
- 文件压缩存储
- 定期清理过期数据
- 存储空间监控

## 9. 错误处理

### 9.1 设备错误
- 摄像头无法访问
- 音频设备无法访问
- 屏幕捕获权限不足

### 9.2 存储错误
- 磁盘空间不足
- 文件写入失败
- 数据库操作失败

### 9.3 网络错误
- API调用失败
- 网络连接中断