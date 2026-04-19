#include "PromptTemplates.h"
#include <sstream>

std::string PromptTemplates::getMemoryClassificationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(分析以下对话内容，判断应该生成哪些类型的记忆：

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请分析并按照以下JSON格式输出结果：
{
  "episodic": true/false,      // 是否应该生成事件记忆（个人经历、事件等）
  "semantic": true/false,      // 是否应该生成语义记忆（事实、概念、知识等）
  "core": true/false,          // 是否应该生成核心记忆（个人偏好、重要信息等）
  "procedural": true/false,    // 是否应该生成程序记忆（操作步骤、方法等）
  "resource": false,           // 资源记忆已被多模态记忆取代，这里固定为false
  "secret": true/false,        // 是否应该生成私密信息库记忆（隐私信息、敏感信息等）
  "confidence": {
    "episodic": 0.0-1.0,
    "semantic": 0.0-1.0,
    "core": 0.0-1.0,
    "procedural": 0.0-1.0,
    "resource": 0.0-1.0,
    "secret": 0.0-1.0
  }
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";

    return prompt.str();
}

std::string PromptTemplates::getEpisodicMemoryGenerationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(请根据以下对话内容提取事件记忆（个人经历、事件等）：

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请按照以下JSON格式输出事件记忆：
{
  "event_description": "事件详细描述（500字以内）",
  "participants": ["参与者1", "参与者2"],
  "location": "事件地点",
  "time_context": "时间上下文",
  "emotional_context": "情感上下文",
  "importance": 0.0-1.0,  // 重要性评分
  "tags": ["标签1", "标签2"]
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
    
    return prompt.str();
}

std::string PromptTemplates::getSemanticMemoryGenerationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(请根据以下对话内容提取语义记忆（事实、概念、知识等）：

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请按照以下JSON格式输出语义记忆：
{
  "fact": "事实或知识点（200字以内）",
  "category": "知识分类（科学、文化、技术等）",
  "confidence": 0.0-1.0,  // 确定性评分
  "relevance_score": 0.0-1.0,  // 相关性评分
  "tags": ["标签1", "标签2"],
  "related_concepts": ["相关概念1", "相关概念2"]
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
    
    return prompt.str();
}

std::string PromptTemplates::getCoreMemoryGenerationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(请根据以下对话内容提取核心记忆（个人偏好、重要信息等）：

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请按照以下JSON格式输出核心记忆：
{
  "preference": "个人偏好或重要信息（200字以内）",
  "category": "信息分类（喜好、习惯、价值观等）",
  "importance": 0.0-1.0,  // 重要性评分
  "confidence": 0.0-1.0,  // 确定性评分
  "tags": ["标签1", "标签2"]
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
    
    return prompt.str();
}

std::string PromptTemplates::getProceduralMemoryGenerationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(请根据以下对话内容提取程序记忆（操作步骤、方法等）：

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请按照以下JSON格式输出程序记忆：
{
  "procedure_name": "操作或方法名称（100字以内）",
  "steps": [
    {
      "step_number": 1,
      "description": "步骤描述"
    }
  ],
  "category": "操作分类（编程、生活技能、工作流程等）",
  "difficulty": "难度等级（简单、中等、困难）",
  "estimated_time": "预估时间",
  "tags": ["标签1", "标签2"]
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
    
    return prompt.str();
}

std::string PromptTemplates::getResourceMemoryGenerationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(请根据以下对话内容提取资源记忆（文件、图片等）：

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请按照以下JSON格式输出资源记忆：
{
  "resource_title": "资源标题（100字以内）",
  "resource_type": "资源类型（图片、文档、视频等）",
  "resource_path": "资源路径",
  "content_summary": "内容摘要（200字以内）",
  "tags": ["标签1", "标签2"],
  "relevance_score": 0.0-1.0  // 相关性评分
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
    
    return prompt.str();
}

std::string PromptTemplates::getSecretVaultGenerationPrompt(
    const std::vector<MessageManager::Message>& messages) {
    std::ostringstream prompt;
    prompt << R"(请根据以下对话内容提取需要存储在私密信息库中的隐私信息或敏感信息:

对话内容：
)";
    
    for (const auto& message : messages) {
        prompt << "[" << message.created_at << "] [" << message.message_type << "] " 
               << message.content << "\n";
    }
    
    prompt << R"(
请按照以下JSON格式输出私密信息库记忆条目:
{
  "title": "私密信息标题（50字以内）",
  "content": "私密信息内容（500字以内）",
  "category": "信息分类（个人身份、财务信息、健康信息、联系方式等）",
  "tags": ["标签1", "标签2"],
  "related_topics": ["主题1", "主题2"],
  "confidence": 0.0-1.0,  // 确定性评分
  "relevance_score": 0.0-1.0  // 相关性评分
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
    
    return prompt.str();
}

std::string PromptTemplates::getImageDescriptionPrompt() {
    return R"(请分析这张图片并按照以下JSON格式提供详细描述:
{
  "main_subject": "图片中的主要对象",
  "scene": "图片场景描述",
  "colors": ["主要颜色1", "主要颜色2"],
  "objects": ["识别出的对象1", "识别出的对象2"],
  "mood": "图片氛围",
  "style": "图片风格",
  "text_content": "图片中的文字内容（如果有）"
}

请严格按照上面的JSON格式输出结果，不要包含其他内容。)";
}