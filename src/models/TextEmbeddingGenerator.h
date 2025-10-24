#ifndef TEXT_EMBEDDING_GENERATOR_H
#define TEXT_EMBEDDING_GENERATOR_H

#include <vector>
#include <string>
#include <memory>
#include "CLIPProcessor.h"

class TextEmbeddingGenerator {
private:
    int embedding_dimension_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;

public:
    TextEmbeddingGenerator();
    std::vector<float> generateEmbedding(const std::string& text);
    int getEmbeddingDimension() const;
};

#endif // TEXT_EMBEDDING_GENERATOR_H