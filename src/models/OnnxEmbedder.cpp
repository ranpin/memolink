#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <array>
#include "OnnxEmbedder.h"
#include <onnxruntime_cxx_api.h>

struct OnnxEmbedder::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_ERROR, "mirix-edge"};
    Ort::SessionOptions opts;
    std::unique_ptr<Ort::Session> session;
    Ort::AllocatorWithDefaultOptions allocator;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
};

OnnxEmbedder::OnnxEmbedder() = default;
OnnxEmbedder::~OnnxEmbedder() = default;

bool OnnxEmbedder::load(const std::string& model_path, int intra_threads) {
    try {
        impl_ = std::make_unique<Impl>();
        impl_->opts.SetIntraOpNumThreads(intra_threads);
        impl_->opts.SetInterOpNumThreads(1);
        impl_->opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        impl_->session = std::make_unique<Ort::Session>(impl_->env, model_path.c_str(), impl_->opts);

        size_t n_in = impl_->session->GetInputCount();
        for (size_t i = 0; i < n_in; ++i) {
            auto nm = impl_->session->GetInputNameAllocated(i, impl_->allocator);
            impl_->input_names.emplace_back(nm.get());
        }
        size_t n_out = impl_->session->GetOutputCount();
        for (size_t i = 0; i < n_out; ++i) {
            auto nm = impl_->session->GetOutputNameAllocated(i, impl_->allocator);
            impl_->output_names.emplace_back(nm.get());
        }
        std::cerr << "[OnnxEmbedder] loaded " << model_path
                  << " inputs=" << impl_->input_names.size()
                  << " outputs=" << impl_->output_names.size() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[OnnxEmbedder] load failed: " << e.what() << std::endl;
        impl_.reset();
        return false;
    }
}

std::vector<std::vector<float>> OnnxEmbedder::run(
    const std::vector<std::vector<int64_t>>& input_ids,
    const std::vector<std::vector<int64_t>>& attention_mask,
    const std::vector<std::vector<int64_t>>& token_type_ids) {

    std::vector<std::vector<float>> result;
    if (!impl_ || !impl_->session) return result;

    const size_t N = input_ids.size();
    if (N == 0) return result;
    const size_t L = input_ids[0].size();
    for (size_t i = 0; i < N; ++i) {
        if (input_ids[i].size() != L) {
            std::cerr << "[OnnxEmbedder] ragged batch (row " << i << " len "
                      << input_ids[i].size() << " != " << L << ")" << std::endl;
            return result;
        }
    }

    // 摊平成连续内存（ONNX 要求 row-major contiguous）
    std::vector<int64_t> ids(N * L), mask(N * L), tt(N * L);
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < L; ++j) {
            ids[i * L + j] = input_ids[i][j];
            mask[i * L + j] = (i < attention_mask.size() && j < attention_mask[i].size())
                                  ? attention_mask[i][j] : 1;
            tt[i * L + j] = (i < token_type_ids.size() && j < token_type_ids[i].size())
                                ? token_type_ids[i][j] : 0;
        }
    }

    try {
        Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        std::array<int64_t, 2> shape{static_cast<int64_t>(N), static_cast<int64_t>(L)};

        // 只喂模型真正声明的输入（不同导出版本可能没有 token_type_ids）
        std::vector<Ort::Value> inputs;
        std::vector<const char*> in_names;
        for (const auto& nm : impl_->input_names) {
            const std::vector<int64_t>* d = nullptr;
            if (nm == "input_ids") d = &ids;
            else if (nm == "attention_mask") d = &mask;
            else if (nm == "token_type_ids") d = &tt;
            if (!d) continue;
            inputs.push_back(Ort::Value::CreateTensor<int64_t>(
                mem, const_cast<int64_t*>(d->data()), d->size(), shape.data(), shape.size()));
            in_names.push_back(nm.c_str());
        }
        std::vector<const char*> out_names;
        for (const auto& nm : impl_->output_names) out_names.push_back(nm.c_str());

        auto outputs = impl_->session->Run(
            Ort::RunOptions{nullptr},
            in_names.data(), inputs.data(), inputs.size(),
            out_names.data(), out_names.size());

        // 优先用模型自带的 sentence_embedding；否则退回 last_hidden_state 的 CLS
        size_t oi = 0;
        for (size_t i = 0; i < impl_->output_names.size(); ++i) {
            if (impl_->output_names[i] == "sentence_embedding") { oi = i; break; }
        }

        auto& t = outputs[oi];
        auto info = t.GetTensorTypeAndShapeInfo();
        auto shp = info.GetShape();
        const float* data = t.GetTensorData<float>();

        int dim = 0;
        std::vector<const float*> rows;
        if (shp.size() == 2) {                       // [N, dim] = sentence_embedding
            dim = static_cast<int>(shp[1]);
            for (size_t i = 0; i < N; ++i) rows.push_back(data + i * dim);
        } else if (shp.size() == 3) {                // [N, seq, dim] = last_hidden_state，取 CLS
            const int seq = static_cast<int>(shp[1]);
            dim = static_cast<int>(shp[2]);
            for (size_t i = 0; i < N; ++i) rows.push_back(data + i * seq * dim);
        } else {
            std::cerr << "[OnnxEmbedder] unexpected output rank " << shp.size() << std::endl;
            return result;
        }

        dim_ = dim;
        result.reserve(N);
        for (const float* p : rows) {
            std::vector<float> v(p, p + dim);
            float ss = 0.f;
            for (float x : v) ss += x * x;
            const float nrm = std::sqrt(ss) + 1e-9f;
            for (float& x : v) x /= nrm;
            result.push_back(std::move(v));
        }
    } catch (const std::exception& e) {
        std::cerr << "[OnnxEmbedder] run failed: " << e.what() << std::endl;
        result.clear();
    }
    return result;
}
