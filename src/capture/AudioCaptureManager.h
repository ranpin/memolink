#pragma once

// AudioCaptureManager - 音频捕获管理器，用于捕获和处理用户的音频输入

#include "../database/core/StorageManager.h"
#include <string>
#include <thread>
#include <atomic>
#include <fstream>
#include <vector>

// 前向声明
typedef void PaStream;

class AudioCaptureManager {
private:
    StorageManager& storage_;
    std::atomic<bool> is_capturing_;
    std::atomic<bool> stop_capture_;
    std::mutex capture_mutex_;
    std::string current_audio_file_;

    // 私有方法
    void captureAudio(const std::string& outputFile, int durationSeconds);

public:
    explicit AudioCaptureManager(StorageManager& storage);
    ~AudioCaptureManager();
    
    bool initialize();
    bool startCapture(const std::string& outputFile, int durationSeconds = 10);
    bool stopCapture();
    bool isCapturing() const;
    std::string getCurrentAudioFile() const;
    
    // 新增函数声明
    int saveAudioToResourceMemory(const std::string& filePath, const std::string& description, const std::string& dbPath);
    
    // 静态方法
    static std::string generateFilename();
    static bool fileExists(const std::string& filename);
};