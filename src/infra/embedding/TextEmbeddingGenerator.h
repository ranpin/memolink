#pragma once
#include <algorithm>

#include "infra/storage/StorageManager.h"
#ifdef MEMOLINK_HAS_CLIP
#include "legacy/infra-alt/CLIPProcessor.h"
#endif
#ifdef MEMOLINK_HAS_EDGE
#include "infra/embedding/OnnxEmbedder.h"
#include "infra/embedding/WordPieceTokenizer.h"
#endif
#include "legacy/infra-alt/ModelManager.h"
#include <string>
#include <vector>
#include <memory>

class TextEmbeddingGenerator {
private:
    StorageManager* storage_;
#ifdef MEMOLINK_HAS_CLIP
    std::unique_ptr<CLIPProcessor> clipProcessor_;
#endif
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
    
    // 构造函数，接受StorageManager指针和CLIP模型路径
    TextEmbeddingGenerator(StorageManager* storage, const std::string& clip_model_path);
    
    // 生成文本的嵌入向量
    std::vector<float> generateTextEmbedding(const std::string& text);
    
    // 生成文本的嵌入向量（使用传入的ModelManager实例）
    std::vector<float> generateTextEmbedding(const std::string& text, ModelManager& modelManager);
    
    // 生成图像描述
    std::string generateImageDescription(const std::string& image_path);
    
    // 生成图像描述（使用传入的ModelManager实例）
    std::string generateImageDescription(const std::string& image_path, ModelManager& modelManager);
    
    // 获取嵌入向量的维度
    int getEmbeddingDimension() const;
    
    // 兼容旧版本的方法名
    std::vector<float> generateEmbedding(const std::string& text) {
        return generateTextEmbedding(text);
    }
    
    std::vector<float> generateEmbedding(const std::string& text, ModelManager& modelManager) {
        return generateTextEmbedding(text, modelManager);
    }
};