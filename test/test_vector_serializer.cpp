#include <cmath>
#include <string>
#include "infra/storage/VectorSerializer.h"
#include <iostream>
#include <vector>
#include <cassert>

int main() {
    std::cout << "测试VectorSerializer功能..." << std::endl;
    
    // 测试空向量
    std::vector<float> emptyVec;
    std::string serializedEmpty = VectorSerializer::serialize(emptyVec);
    std::vector<float> deserializedEmpty = VectorSerializer::deserialize(serializedEmpty);
    assert(deserializedEmpty.empty());
    std::cout << "✓ 空向量测试通过" << std::endl;
    
    // 测试普通向量
    std::vector<float> testVec = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::string serialized = VectorSerializer::serialize(testVec);
    std::vector<float> deserialized = VectorSerializer::deserialize(serialized);
    
    assert(testVec.size() == deserialized.size());
    for (size_t i = 0; i < testVec.size(); ++i) {
        assert(std::abs(testVec[i] - deserialized[i]) < 1e-6);
    }
    std::cout << "✓ 普通向量测试通过" << std::endl;
    
    // 测试包含负数和小数的向量
    std::vector<float> testVec2 = {-1.5f, 0.0f, 1.5f, -2.7f, 3.14f};
    std::string serialized2 = VectorSerializer::serialize(testVec2);
    std::vector<float> deserialized2 = VectorSerializer::deserialize(serialized2);
    
    assert(testVec2.size() == deserialized2.size());
    for (size_t i = 0; i < testVec2.size(); ++i) {
        assert(std::abs(testVec2[i] - deserialized2[i]) < 1e-6);
    }
    std::cout << "✓ 负数和小数向量测试通过" << std::endl;
    
    // 测试大型向量
    std::vector<float> largeVec(1000);
    for (size_t i = 0; i < largeVec.size(); ++i) {
        largeVec[i] = static_cast<float>(i) * 0.1f;
    }
    std::string serializedLarge = VectorSerializer::serialize(largeVec);
    std::vector<float> deserializedLarge = VectorSerializer::deserialize(serializedLarge);
    
    assert(largeVec.size() == deserializedLarge.size());
    for (size_t i = 0; i < largeVec.size(); ++i) {
        assert(std::abs(largeVec[i] - deserializedLarge[i]) < 1e-6);
    }
    std::cout << "✓ 大型向量测试通过" << std::endl;
    
    std::cout << "所有VectorSerializer测试通过！" << std::endl;
    return 0;
}