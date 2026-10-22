#include <vector>
#include <string>
#include "infra/storage/VectorSerializer.h"
#include <cstring>

std::string VectorSerializer::serialize(const std::vector<float>& vec) {
    if (vec.empty()) {
        return std::string();
    }
    
    // 直接将浮点数向量的内存数据复制到字符串中
    const char* data = reinterpret_cast<const char*>(vec.data());
    size_t size = vec.size() * sizeof(float);
    return std::string(data, size);
}

std::vector<float> VectorSerializer::deserialize(const std::string& data) {
    std::vector<float> vec;
    
    if (data.empty()) {
        return vec;
    }
    
    // 计算浮点数的个数
    size_t floatCount = data.size() / sizeof(float);
    if (floatCount * sizeof(float) != data.size()) {
        // 数据大小不匹配，可能已损坏
        return vec;
    }
    
    // 调整向量大小
    vec.resize(floatCount);
    
    // 将字符串数据复制到浮点数向量中
    std::memcpy(vec.data(), data.data(), data.size());
    
    return vec;
}