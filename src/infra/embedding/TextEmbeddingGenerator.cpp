#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include "infra/embedding/TextEmbeddingGenerator.h"
#include "infra/llm/QwenAPI.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "infra/storage/ConfigManager.h"

TextEmbeddingGenerator::TextEmbeddingGenerator(StorageManager* storage) : storage_(storage) {
    std::cout << "[TextEmbeddingGenerator] 初始化..." << std::endl;

    // 获取配置管理器实例
    ConfigManager& config_manager = ConfigManager::getInstance();

    // 获取配置的模型和维度
    std::string model_name = config_manager.getEmbeddingModelName();

    // 根据模型名称设置维度
    if (model_name == "text-embedding-v4") {
        embedding_dimension_ = 1024; // text-embedding-v4模型维度
    } else if (model_name == "text-embedding-v1") {
        embedding_dimension_ = 1536; // text-embedding-v1模型维度
    } else {
        embedding_dimension_ = 128; // 默认维度
    }

    std::cout << "[TextEmbeddingGenerator] 配置的模型: " << model_name << std::endl;
    std::cout << "[TextEmbeddingGenerator] 设置维度为" << embedding_dimension_ << " (" << model_name << ")" << std::endl;

#ifdef MEMOLINK_HAS_EDGE
    // 端侧本地 embedding：config.ini [edge] 两个路径都配上才启用。
    // 启用后 generateTextEmbedding 走本地 ONNX，不再打云端 text-embedding 接口。
    {
        const std::string model_path = config_manager.getEdgeEmbeddingModelPath();
        const std::string tok_path = config_manager.getEdgeTokenizerPath();
        if (!model_path.empty() && !tok_path.empty()) {
            edgeTokenizer_ = std::make_unique<WordPieceTokenizer>();
            edgeEmbedder_ = std::make_unique<OnnxEmbedder>();
            if (edgeTokenizer_->load(tok_path) &&
                edgeEmbedder_->load(model_path, config_manager.getEdgeEmbeddingThreads())) {
                edge_ready_ = true;
                embedding_dimension_ = edgeEmbedder_->dim();  // 以实际模型维度为准
                std::cout << "[TextEmbeddingGenerator] 端侧 ONNX embedding 就绪，维度 "
                          << embedding_dimension_ << std::endl;
            } else {
                std::cerr << "[TextEmbeddingGenerator] 端侧 embedding 加载失败，回退云端/模拟" << std::endl;
                edgeTokenizer_.reset();
                edgeEmbedder_.reset();
            }
        }
    }
#endif

    std::cout << "[TextEmbeddingGenerator] 初始化完成，配置维度: " << embedding_dimension_ << std::endl;
}

std::vector<float> TextEmbeddingGenerator::generateTextEmbedding(const std::string& text) {
    std::cout << "[TextEmbeddingGenerator] 开始生成文本embedding..." << std::endl;
    std::cout << "[TextEmbeddingGenerator] 输入文本长度: " << text.length() << " 字符" << std::endl;

#ifdef MEMOLINK_HAS_EDGE
    // 端侧本地 ONNX embedding 优先（config.ini [edge] 两个路径都配上才启用）。
    // 注：BGE 建议 query 侧加检索指令前缀，但本接口无法区分 query/memory，
    // 故统一不加前缀（对称编码）；前缀优化留到 P4 检索评测按需引入。
    if (edge_ready_) {
        std::vector<int> ids = edgeTokenizer_->encode(text, 128);
        std::vector<std::vector<int64_t>> ids_b{std::vector<int64_t>(ids.begin(), ids.end())};
        std::vector<std::vector<int64_t>> mask_b{std::vector<int64_t>(ids.size(), 1)};
        std::vector<std::vector<int64_t>> tt_b{std::vector<int64_t>(ids.size(), 0)};
        std::vector<std::vector<float>> vecs = edgeEmbedder_->run(ids_b, mask_b, tt_b);
        if (!vecs.empty() && !vecs[0].empty()) {
            std::cout << "[TextEmbeddingGenerator] 端侧 ONNX embedding 成功，维度: "
                      << vecs[0].size() << std::endl;
            return vecs[0];
        }
        std::cerr << "[TextEmbeddingGenerator] 端侧 embedding 失败，回退云端/模拟" << std::endl;
    }
#endif

    // 优先使用text-embedding-v4模型生成文本embedding（通过Qwen API）
    const char* api_key = std::getenv("DASHSCOPE_API_KEY");
    if (api_key && strlen(api_key) > 0) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用text-embedding-v4模型生成文本embedding..." << std::endl;
        try {
            // 调用Qwen API的text-embedding-v4模型
            QwenAPI qwen_api;
            qwen_api.set_api_key(api_key);
            QwenEmbeddingRequest request;
            request.model = "text-embedding-v4";
            request.input.push_back(text);
            QwenEmbeddingResponse response = qwen_api.generate_embeddings(request);

            if (!response.data.empty()) {
                std::vector<float> embedding = response.data[0].embedding;
                // 确保维度匹配
                if (embedding.size() != embedding_dimension_) {
                    std::cout << "[TextEmbeddingGenerator] 警告: 生成的embedding维度(" << embedding.size()
                             << ")与配置维度(" << embedding_dimension_ << ")不匹配" << std::endl;
                }
                std::cout << "[TextEmbeddingGenerator] text-embedding-v4模型生成文本embedding成功，维度: " << embedding.size() << std::endl;
                return embedding;
            } else {
                throw std::runtime_error("Empty embedding response");
            }
        }

        // 如果API调用失败，回退到模拟实现
        catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] 警告: text-embedding-v4模型调用失败: " << e.what() << std::endl;
            std::cerr << "[TextEmbeddingGenerator] 回退到模拟实现" << std::endl;
        }
    } else {
        std::cout << "[TextEmbeddingGenerator] 未配置API密钥，跳过text-embedding-v4模型" << std::endl;
    }

    // 生成模拟的嵌入向量
    std::cout << "[TextEmbeddingGenerator] 使用模拟实现生成embedding，维度: " << embedding_dimension_ << std::endl;
    std::vector<float> embedding(embedding_dimension_, 0.0f);

    // 简单的文本特征映射到嵌入向量
    for (size_t i = 0; i < text.length() && i < embedding.size(); ++i) {
        embedding[i] = std::tanh(static_cast<float>(text[i]) / 128.0f); // 使用tanh归一化
    }

    std::cout << "[TextEmbeddingGenerator] 模拟embedding生成完成" << std::endl;
    return embedding;
}

int TextEmbeddingGenerator::getEmbeddingDimension() const {
    return embedding_dimension_;
}
