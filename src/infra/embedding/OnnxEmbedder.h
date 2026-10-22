#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// bge-small-zh 的端侧 embedding 推理，走 ONNX Runtime 原生 C++ API（无 Python 运行时）。
//
// 用 pImpl 把 onnxruntime 的头隔离在 .cpp 里——include 本头文件不需要 onnxruntime
// 的 include 路径，只有 OnnxEmbedder.cpp 需要。这样记忆层（2618 行 manager）完全
// 不感知 onnxruntime 的存在。
//
// 刻意跑 CPU：把 GPU 整块留给 LLM（F1 实测 INT8 单线程 10.8ms 已够，见 docs/orin_edge_plan.md）。
class OnnxEmbedder {
public:
    OnnxEmbedder();
    ~OnnxEmbedder();
    OnnxEmbedder(const OnnxEmbedder&) = delete;
    OnnxEmbedder& operator=(const OnnxEmbedder&) = delete;

    // 加载 ONNX 模型。external data（model.onnx_data）必须与 .onnx 同目录同名。
    bool load(const std::string& model_path, int intra_threads = 4);

    // 跑一个 batch。三个输入都是 [N][L]、已 padding 到同一长度 L。
    // 返回 L2 归一化后的 [N][dim]；失败返回空 vector。
    std::vector<std::vector<float>> run(
        const std::vector<std::vector<int64_t>>& input_ids,
        const std::vector<std::vector<int64_t>>& attention_mask,
        const std::vector<std::vector<int64_t>>& token_type_ids);

    int dim() const { return dim_; }
    bool loaded() const { return static_cast<bool>(impl_); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    int dim_ = 0;
};
