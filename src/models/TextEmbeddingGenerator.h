#ifndef TEXT_EMBEDDING_GENERATOR_H
#define TEXT_EMBEDDING_GENERATOR_H

#include <vector>
#include <string>
#include <memory>
#include "CLIPProcessor.h"
#include "ModelManager.h"
#include "../database/core/StorageManager.h"

class TextEmbeddingGenerator {
private:
    int embedding_dimension_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;
    StorageManager* storage_;

public:
    TextEmbeddingGenerator(StorageManager* storage = nullptr);
    std::vector<float> generateEmbedding(const std::string& text);
    std::string generateImageDescription(const std::string& image_path);
    int getEmbeddingDimension() const;
};

#endif // TEXT_EMBEDDING_GENERATOR_H