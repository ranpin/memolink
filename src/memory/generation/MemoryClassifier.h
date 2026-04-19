#ifndef MEMORY_CLASSIFIER_H
#define MEMORY_CLASSIFIER_H

#include <string>
#include <vector>
#include "../messages/MessageManager.h"

class MemoryClassifier {
public:
    struct MemoryTypePrediction {
        bool episodic = false;
        bool semantic = false;
        bool core = false;
        bool procedural = false;
        bool resource = false;
        bool secret = false;
        float episodic_confidence = 0.0f;
        float semantic_confidence = 0.0f;
        float core_confidence = 0.0f;
        float procedural_confidence = 0.0f;
        float resource_confidence = 0.0f;
        float secret_confidence = 0.0f;
    };

    static MemoryTypePrediction classify(const std::vector<MessageManager::Message>& messages);
    
private:
    static std::string buildClassificationPrompt(const std::vector<MessageManager::Message>& messages);
    static MemoryTypePrediction parseClassificationResult(const std::string& jsonResult);
};

#endif // MEMORY_CLASSIFIER_H