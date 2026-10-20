#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include "models/TextEmbeddingGenerator.h"
#include "database/initialization/ConfigManager/ConfigManager.h"

// P2 接线验证：TextEmbeddingGenerator 是否真的走上了本地 ONNX。
//
// 前置：config.ini 的 [edge] 段要填好 embedding_model_path 与 tokenizer_path
// （仓库里默认留空 = 不启用，保持可移植；在设备上按实际路径填）。
//
// 判据：每条文本返回 512 维、L2 归一化（norm≈1）的向量。
// 若 edge 未启用，会回退到云端/模拟实现，维度就不是 512、norm 也不为 1，测试即失败。
int main() {
    ConfigManager::getInstance();  // 触发 loadConfig（按 ../config.ini 等顺序查找）

    const std::string model_path = ConfigManager::getInstance().getEdgeEmbeddingModelPath();
    const std::string tok_path = ConfigManager::getInstance().getEdgeTokenizerPath();
    std::cout << "[test] edge.embedding_model_path = " << (model_path.empty() ? "(空)" : model_path) << std::endl;
    std::cout << "[test] edge.tokenizer_path       = " << (tok_path.empty() ? "(空)" : tok_path) << std::endl;
    if (model_path.empty() || tok_path.empty()) {
        std::cerr << "[test] FAIL: config.ini 的 [edge] 路径未配置，无法验证本地 ONNX 接线" << std::endl;
        return 2;
    }

    TextEmbeddingGenerator gen(nullptr);
    std::cout << "[test] getEmbeddingDimension() = " << gen.getEmbeddingDimension() << std::endl;

    const std::vector<std::string> tests = {
        "用户喜欢深蓝色的车",
        "空调调到22度",
        "hello world",
        "车辆上次保养是 2026 年 8 月，里程 12000 公里。",
        "我有点冷",
    };

    int ok = 0;
    for (const auto& t : tests) {
        std::vector<float> emb = gen.generateTextEmbedding(t);
        float ss = 0.f;
        for (float x : emb) ss += x * x;
        const float norm = std::sqrt(ss);
        const bool good = (emb.size() == 512) && (std::fabs(norm - 1.0f) < 0.01f);
        if (good) ++ok;
        std::cout << "  " << (good ? "✅" : "❌") << " dim=" << emb.size()
                  << " norm=" << norm
                  << " first3=[" << (emb.size() > 0 ? emb[0] : 0.f) << ", "
                  << (emb.size() > 1 ? emb[1] : 0.f) << ", "
                  << (emb.size() > 2 ? emb[2] : 0.f) << "]"
                  << "  text='" << t << "'" << std::endl;
    }

    std::cout << "[test] PASS " << ok << "/" << tests.size() << std::endl;
    return (ok == static_cast<int>(tests.size())) ? 0 : 1;
}
