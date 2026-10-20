#include <algorithm>
#include <stdexcept>
#include <string>
#include "QwenAPI.h"
#include "../database/initialization/ConfigManager/ConfigManager.h"
#include <curl/curl.h>
#include <json/json.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <vector>

// 回调函数，用于curl写入数据
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

// Base64编码字符表
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// Base64编码函数
std::string base64_encode(const std::vector<unsigned char>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    auto it = data.begin();
    while (it != data.end()) {
        char_array_3[i++] = *(it++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

// 将文件转换为base64编码
std::string file_to_base64(const std::string& file_path) {
    std::cout << "[Base64编码] 开始处理文件: " << file_path << std::endl;
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + file_path);
    }
    
    // 检查文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 限制文件大小为10MB
    if (file_size > 10 * 1024 * 1024) {
        file.close();
        throw std::runtime_error("文件过大: " + std::to_string(file_size) + " 字节，超过10MB限制");
    }
    
    std::cout << "[Base64编码] 文件打开成功" << std::endl;
    std::cout << "[Base64编码] 文件大小: " << file_size << " 字节" << std::endl;
    
    std::vector<unsigned char> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    
    // 检查读取是否成功
    if (file.gcount() != static_cast<std::streamsize>(file_size)) {
        file.close();
        throw std::runtime_error("文件读取不完整: 期望读取 " + std::to_string(file_size) + 
                                " 字节，实际读取 " + std::to_string(file.gcount()) + " 字节");
    }
    
    file.close();
    
    std::cout << "[Base64编码] 文件读取成功，大小: " << file_size << " 字节" << std::endl;
    
    std::string encoded_data = base64_encode(buffer);
    std::cout << "[Base64编码] 编码完成，编码后长度: " << encoded_data.length() << std::endl;
    // 只输出前100个字符，避免日志过长
    std::cout << "[Base64编码] 编码数据(前100字符): " << encoded_data.substr(0, 100) << std::endl;
    
    return encoded_data;
}

QwenAPI::QwenAPI() : base_url_("https://dashscope.aliyuncs.com"), use_local_api_(false) {
}

QwenAPI::~QwenAPI() {
}

bool QwenAPI::shouldUseLocalAPI() const {
    // 检查是否配置了本地API URL
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string local_api_url = config_manager.getLocalApiUrl();
    
    // 如果配置了本地API URL且不为空，则使用本地API
    return !local_api_url.empty();
}

void QwenAPI::set_api_key(const std::string& api_key) {
    api_key_ = api_key;
}

void QwenAPI::set_base_url(const std::string& base_url) {
    base_url_ = base_url;
}

QwenChatCompletionResponse QwenAPI::chat_completion(const QwenChatCompletionRequest& request) {
    CURL* curl = curl_easy_init();
    QwenChatCompletionResponse response;
    
    if (curl) {
        // 确定使用的API端点
        std::string url;
        if (shouldUseLocalAPI()) {
            // 使用本地API
            ConfigManager& config_manager = ConfigManager::getInstance();
            url = config_manager.getLocalApiUrl() + "/v1/chat/completions";
        } else {
            // 使用阿里云DashScope API
            url = base_url_ + "/compatible-mode/v1/chat/completions";
        }
        
        // 检查是否使用需要流式传输的模型
        // 修复：不再写死模型名称，而是检查模型名称是否包含"omni"关键词
        bool needs_streaming = (request.model.find("omni") != std::string::npos);
        
        // 构建JSON请求体
        Json::Value root;
        root["model"] = request.model;
        
        Json::Value messages(Json::arrayValue);
        for (const auto& msg : request.messages) {
            Json::Value message;
            message["role"] = msg.role;
            
            if (!msg.contents.empty()) {
                // 处理多模态内容
                Json::Value contents(Json::arrayValue);
                for (const auto& content : msg.contents) {
                    Json::Value c;
                    if (content.type == "image_url") {
                        // 直接使用已构造的base64数据URL，不再重复处理
                        c["type"] = "image_url";
                        Json::Value image_url_obj;
                        image_url_obj["url"] = content.image_url;  // 这里已经是完整的data:image/...;base64,...格式
                        c["image_url"] = image_url_obj;
                    } else if (content.type == "text") {
                        c["type"] = "text";
                        c["text"] = content.text;
                    } else if (content.type == "audio_url") {
                        // 将音频文件转换为base64
                        std::string base64_data = file_to_base64(content.audio_url);
                        if (!base64_data.empty()) {
                            // 使用text类型包含音频数据描述
                            c["type"] = "text";
                            c["text"] = "[音频数据已提供，但当前API不直接支持音频输入]";
                        } else {
                            std::cerr << "音频文件base64编码失败: " << content.audio_url << std::endl;
                            c["type"] = "text";
                            c["text"] = "[音频文件不可用: " + content.audio_url + "]";
                        }
                    } else {
                        // 默认处理
                        c["type"] = "text";
                        c["text"] = "[未知内容类型]";
                    }
                    contents.append(c);
                }
                message["content"] = contents;
            } else {
                // 处理纯文本内容
                message["content"] = msg.content;
            }
            
            messages.append(message);
        }
        root["messages"] = messages;
        root["temperature"] = request.temperature;
        root["top_p"] = request.top_p;
        root["max_tokens"] = request.max_tokens;
        
        // 对于需要流式传输的模型，设置stream=true
        if (needs_streaming) {
            root["stream"] = true;
        }
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = ""; // 紧凑格式
        std::string json_body = Json::writeString(builder, root);
        
        
        // 设置HTTP头
        struct curl_slist* headers = nullptr;
        if (shouldUseLocalAPI()) {
            // 本地API可能不需要特定的认证头，或者使用简单的Bearer token
            if (!api_key_.empty()) {
                headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
            }
        } else {
            // 阿里云DashScope需要特定的认证头
            headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
            headers = curl_slist_append(headers, "X-DashScope-SSE: disable");
        }
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // 设置curl选项
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_body.length());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        
        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        
        // 设置超时
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // 执行请求
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            response.content = "Error: Failed to call API: " + std::string(curl_easy_strerror(res));
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            
            // 如果是流式响应，需要特殊处理
            if (needs_streaming) {
                // 解析流式响应，提取最终内容
                std::string final_content;
                std::istringstream response_stream(response_string);
                std::string line;
                
                while (std::getline(response_stream, line)) {
                    // 跳过空行和data: [DONE]行
                    if (line.empty() || line == "data: [DONE]") {
                        continue;
                    }
                    
                    // 移除"data: "前缀
                    if (line.substr(0, 6) == "data: ") {
                        line = line.substr(6);
                    }
                    
                    // 解析JSON
                    Json::CharReaderBuilder reader_builder;
                    Json::Value chunk_json;
                    std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
                    Json::String errs;
                    
                    bool parsingSuccessful = reader->parse(line.c_str(), 
                                                         line.c_str() + line.size(),
                                                         &chunk_json, &errs);
                    
                    if (parsingSuccessful) {
                        if (chunk_json.isMember("choices") && chunk_json["choices"].isArray() && 
                            chunk_json["choices"].size() > 0) {
                            Json::Value choice = chunk_json["choices"][0];
                            if (choice.isMember("delta") && choice["delta"].isMember("content")) {
                                final_content += choice["delta"]["content"].asString();
                            }
                        }
                    }
                }
                
                response.content = final_content;
            } else {
                // 解析非流式响应
                Json::CharReaderBuilder reader_builder;
                Json::Value response_json;
                std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
                Json::String errs;
                
                bool parsingSuccessful = reader->parse(response_string.c_str(), 
                                                     response_string.c_str() + response_string.size(),
                                                     &response_json, &errs);
                
                if (parsingSuccessful) {
                    // 添加类型检查
                    if (response_json.isObject()) {
                        if (response_json.isMember("choices") && response_json["choices"].isArray() && 
                            response_json["choices"].size() > 0) {
                            Json::Value choice = response_json["choices"][0];
                            // 确保choice是对象类型
                            if (choice.isObject()) {
                                if (choice.isMember("message") && choice["message"].isMember("content")) {
                                    response.content = choice["message"]["content"].asString();
                                } else if (choice.isMember("delta") && choice["delta"].isMember("content")) {
                                    response.content = choice["delta"]["content"].asString();
                                }
                            }
                        } else if (response_json.isMember("error")) {
                            // 确保error是对象类型
                            if (response_json["error"].isObject() && response_json["error"].isMember("message")) {
                                response.content = "Error: " + response_json["error"]["message"].asString();
                            } else {
                                response.content = "Error: " + response_json["error"].asString();
                            }
                        } else {
                            response.content = "Error: Invalid response format";
                        }
                    } else {
                        response.content = "Error: Response is not a JSON object";
                    }
                } else {
                    response.content = "Error: Failed to parse API response: " + errs;
                    // 添加响应内容以便调试
                    std::cerr << "Response content: " << response_string << std::endl;
                }
            }
        }
        
        // 清理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return response;
    }
    
    // 默认响应
    response.content = "Error: Failed to initialize CURL";
    return response;
}

QwenEmbeddingResponse QwenAPI::generate_embeddings(const QwenEmbeddingRequest& request) {
    CURL* curl = curl_easy_init();
    QwenEmbeddingResponse response;
    
    if (curl) {
        // 确定使用的API端点
        std::string url;
        if (shouldUseLocalAPI()) {
            // 使用本地API
            ConfigManager& config_manager = ConfigManager::getInstance();
            url = config_manager.getLocalApiUrl() + "/v1/embeddings";
        } else {
            // 使用阿里云DashScope API
            url = base_url_ + "/compatible-mode/v1/embeddings";
        }
        
        // 构建JSON请求体
        Json::Value root;
        root["model"] = request.model;
        
        Json::Value input(Json::arrayValue);
        for (const auto& text : request.input) {
            input.append(text);
        }
        root["input"] = input;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = ""; // 紧凑格式
        std::string json_body = Json::writeString(builder, root);
        
        
        // 设置HTTP头
        struct curl_slist* headers = nullptr;
        if (shouldUseLocalAPI()) {
            // 本地API可能不需要特定的认证头，或者使用简单的Bearer token
            if (!api_key_.empty()) {
                headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
            }
        } else {
            // 阿里云DashScope需要特定的认证头
            headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        }
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // 设置curl选项
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_body.length());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        
        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        
        // 设置超时
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // 执行请求
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            response.message = "Error: Failed to call Embedding API: " + std::string(curl_easy_strerror(res));
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            
            // 解析响应
            Json::CharReaderBuilder reader_builder;
            Json::Value response_json;
            std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
            Json::String errs;
            
            bool parsingSuccessful = reader->parse(response_string.c_str(), 
                                                 response_string.c_str() + response_string.size(),
                                                 &response_json, &errs);
            
            if (parsingSuccessful) {
                // 添加类型检查
                if (response_json.isObject()) {
                    if (response_json.isMember("data") && response_json["data"].isArray()) {
                        for (const auto& item : response_json["data"]) {
                            // 确保item是对象类型
                            if (item.isObject()) {
                                QwenEmbeddingData data;
                                data.index = item.isMember("index") ? item["index"].asInt() : 0;
                                
                                Json::Value embedding = item["embedding"];
                                if (embedding.isArray()) {
                                    for (const auto& val : embedding) {
                                        data.embedding.push_back(val.asFloat());
                                    }
                                }
                                
                                response.data.push_back(data);
                            }
                        }
                    } else if (response_json.isMember("error")) {
                        // 确保error是对象类型
                        if (response_json["error"].isObject() && response_json["error"].isMember("message")) {
                            response.message = "Error: " + response_json["error"]["message"].asString();
                        } else {
                            response.message = "Error: " + response_json["error"].asString();
                        }
                    } else {
                        response.message = "Error: Invalid response format";
                    }
                } else {
                    response.message = "Error: Response is not a JSON object";
                }
            } else {
                response.message = "Error: Failed to parse API response: " + errs;
                // 添加响应内容以便调试
                std::cerr << "Response content: " << response_string << std::endl;
            }
        }
        
        // 清理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return response;
    }
    
    // 默认响应
    response.message = "Error: Failed to initialize CURL";
    return response;
}