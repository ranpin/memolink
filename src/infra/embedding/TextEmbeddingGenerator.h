#pragma once
#include <algorithm>

#include "infra/storage/StorageManager.h"
#ifdef MEMOLINK_HAS_EDGE
#include "infra/embedding/OnnxEmbedder.h"
#include "infra/embedding/WordPieceTokenizer.h"
#endif
#include <string>
#include <vector>
#include <memory>

// 文本 embedding 生成器（v2 端侧主链只用单参 generateTextEmbedding(text)）。
// 2026-10-22 起与 legacy 解耦：原对 legacy/infra-alt 的 ModelManager / CLIPProcessor
// 依赖（多模态图像描述、云端 omni 路径）随桌面多模态系统一起移出构建，见
// docs/architecture.md「v1 设计遗产」。
class TextEmbeddingGenerator {
private:
    StorageManager* storage_;
#ifdef MEMOLINK_HAS_EDGE
    // 端侧本地 embedding：自写 WordPiece 分词 + ONNX Runtime 原生推理。
    // config.ini 的 [edge] 两个路径都配上才会启用；启用后 generateTextEmbedding
    // 走本地、不再打云端 DashScope text-embedding 接口。
    std::unique_ptr<WordPieceTokenizer> edgeTokenizer_;
    std::unique_ptr<OnnxEmbedder> edgeEmbedder_;
    bool edge_ready_ = false;
#endif
    int embedding_dimension_;

public:
    // 构造函数，接受StorageManager指针（可为空）
    explicit TextEmbeddingGenerator(StorageManager* storage);

    // 生成文本的嵌入向量
    std::vector<float> generateTextEmbedding(const std::string& text);

    // 获取嵌入向量的维度
    int getEmbeddingDimension() const;

    // 兼容旧版本的方法名
    std::vector<float> generateEmbedding(const std::string& text) {
        return generateTextEmbedding(text);
    }
};
