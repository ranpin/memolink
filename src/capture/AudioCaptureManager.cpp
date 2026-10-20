#include <vector>
#include <string>
#include "AudioCaptureManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "../utils/TimeUtils.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <portaudio.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <filesystem>

// WAV文件头结构
struct WAVHeader {
    char riff_header[4]; // "RIFF"
    int wav_size;
    char wave_header[4]; // "WAVE"
    char fmt_header[4];  // "fmt "
    int fmt_chunk_size;
    short audio_format;
    short num_channels;
    int sample_rate;
    int byte_rate;
    short block_align;
    short bits_per_sample;
    char data_header[4]; // "data"
    int data_bytes;
};

// 添加互斥锁以确保线程安全
static std::mutex g_audio_mutex;

AudioCaptureManager::AudioCaptureManager(StorageManager& storage) 
    : storage_(storage), is_capturing_(false), stop_capture_(false) {
    // 初始化PortAudio
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio初始化失败: " << Pa_GetErrorText(err) << std::endl;
    }
}

AudioCaptureManager::~AudioCaptureManager() {
    // 终止PortAudio
    stopCapture();
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    Pa_Terminate();
}

bool AudioCaptureManager::initialize() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio初始化失败: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    return true;
}

bool AudioCaptureManager::startCapture(const std::string& outputFile, int durationSeconds) {
    if (is_capturing_.load()) {
        std::cerr << "音频捕获已在进行中" << std::endl;
        return false;
    }

    current_audio_file_ = outputFile;
    is_capturing_ = true;
    stop_capture_ = false;

    // 在单独的线程中执行捕获
    std::thread captureThread([this, outputFile, durationSeconds]() {
        this->captureAudio(outputFile, durationSeconds);
    });
    captureThread.detach();

    return true;
}

void AudioCaptureManager::captureAudio(const std::string& outputFile, int durationSeconds) {
    // 确保输出目录存在
    std::filesystem::path outputPath(outputFile);
    std::filesystem::create_directories(outputPath.parent_path());

    // 创建WAV文件头
    WAVHeader header;
    std::memcpy(header.riff_header, "RIFF", 4);
    std::memcpy(header.wave_header, "WAVE", 4);
    std::memcpy(header.fmt_header, "fmt ", 4);
    std::memcpy(header.data_header, "data", 4);

    header.wav_size = 36 + 44100 * 2 * durationSeconds; // 简化的大小计算
    header.fmt_chunk_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = 1; // 单声道
    header.sample_rate = 44100;
    header.byte_rate = 44100 * 1 * 2; // sample_rate * num_channels * bits_per_sample/8
    header.block_align = 1 * 2; // num_channels * bits_per_sample/8
    header.bits_per_sample = 16;
    header.data_bytes = 44100 * 2 * durationSeconds;

    // 打开输出文件
    std::ofstream audioFile(outputFile, std::ios::binary);
    if (!audioFile.is_open()) {
        std::cerr << "无法打开音频文件进行写入: " << outputFile << std::endl;
        is_capturing_ = false;
        return;
    }

    // 写入WAV头
    audioFile.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

    // 初始化PortAudio流
    PaStream* audioStream = nullptr;
    PaError err;
    
    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        std::cerr << "未找到默认音频输入设备" << std::endl;
        audioFile.close();
        is_capturing_ = false;
        return;
    }

    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&audioStream,
                        &inputParameters,
                        nullptr, // 没有输出参数
                        44100,
                        paFramesPerBufferUnspecified,
                        paClipOff,
                        nullptr, // 没有回调函数
                        nullptr);
    
    if (err != paNoError) {
        std::cerr << "打开音频流失败: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(audioStream);
        audioStream = nullptr;
        audioFile.close();
        is_capturing_ = false;
        return;
    }

    err = Pa_StartStream(audioStream);
    if (err != paNoError) {
        std::cerr << "启动音频流失败: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(audioStream);
        audioStream = nullptr;
        audioFile.close();
        is_capturing_ = false;
        return;
    }

    // 捕获音频数据
    const int framesPerBuffer = 512;
    std::vector<short> buffer(framesPerBuffer);
    auto startTime = std::chrono::steady_clock::now();

    while (is_capturing_.load() && !stop_capture_.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        if (elapsed >= durationSeconds) {
            break;
        }

        err = Pa_ReadStream(audioStream, buffer.data(), framesPerBuffer);
        if (err != paNoError) {
            std::cerr << "读取音频流失败: " << Pa_GetErrorText(err) << std::endl;
            break;
        }

        // 写入音频数据到文件
        audioFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(short));
    }

    // 停止并关闭音频流
    Pa_StopStream(audioStream);
    Pa_CloseStream(audioStream);
    audioStream = nullptr;
    audioFile.close();
    is_capturing_ = false;
}

bool AudioCaptureManager::stopCapture() {
    if (!is_capturing_.load()) {
        return false;
    }
    
    stop_capture_ = true;
    return true;
}

bool AudioCaptureManager::isCapturing() const {
    return is_capturing_.load();
}

std::string AudioCaptureManager::getCurrentAudioFile() const {
    return current_audio_file_;
}

int AudioCaptureManager::saveAudioToResourceMemory(const std::string& filePath, const std::string& description, const std::string& dbPath) {
    // 获取文件大小
    std::ifstream in_file(filePath, std::ios::binary | std::ios::ate);
    if (!in_file.is_open()) {
        std::cerr << "无法打开文件: " << filePath << std::endl;
        return -1;
    }
    
    int file_size = static_cast<int>(in_file.tellg());
    in_file.close();
    
    // 使用ResourceMemoryManager存储音频数据
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "无法初始化数据库: " << dbPath << std::endl;
        return -1;
    }
    
    ResourceMemoryManager resourceManager(storage);
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = 1;
    item.agent_id = 1;
    item.description = description.empty() ? "Captured audio: " + filePath : description;
    item.resource_type = "audio";
    item.path = filePath;
    item.metadata = "{}";
    item.size = 0;  // 默认大小为0
    item.embedding = "";
    item.created_at = "";
    
    if (resourceManager.add_resource_memory(item)) {
        return item.id;
    }
    
    return -1;
}

// 静态方法实现
std::string AudioCaptureManager::generateFilename() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

bool AudioCaptureManager::fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}
