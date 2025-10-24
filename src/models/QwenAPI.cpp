#include "QwenAPI.h"
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

// 将文件转换为base64编码
std::string file_to_base64(const std::string& file_path) {
    // 检查是否已经是base64数据URL格式
    if (file_path.substr(0, 5) == "data:") {
        // 如果已经是data URL格式，直接提取base64部分
        size_t comma_pos = file_path.find(',');
        if (comma_pos != std::string::npos) {
            // 返回逗号后的base64数据部分
            return file_path.substr(comma_pos + 1);
        }
        std::cerr << "无效的data URL格式: " << file_path << std::endl;
        return "";
    }
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件进行base64编码: " << file_path << std::endl;
        return "";
    }
    
    // 读取文件内容
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    file.close();
    
    if (buffer.empty()) {
        std::cerr << "文件为空: " << file_path << std::endl;
        return "";
    }
    
    // 简单的base64编码实现
    static const char* base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
    
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (unsigned char c : buffer) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (j = 0; j < i + 1; j++) {
            ret += base64_chars[char_array_4[j]];
        }
        
        while (i++ < 3) {
            ret += '=';
        }
    }
    
    return ret;
}

QwenAPI::QwenAPI() : base_url_("https://dashscope.aliyuncs.com") {
}

QwenAPI::~QwenAPI() {
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
        // 构建请求URL - 使用DashScope兼容模式端点
        std::string url = base_url_ + "/compatible-mode/v1/chat/completions";
        
        // 检查是否使用需要流式传输的模型
        bool needs_streaming = (request.model == "qwen2.5-omni-7b");
        
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
                        // 将图像文件转换为base64
                        std::string base64_data = file_to_base64(content.image_url);
                        if (!base64_data.empty()) {
                            // 使用DashScope支持的格式
                            Json::Value image_url;
                            image_url["url"] = "data:image/jpeg;base64," + base64_data;
                            c["type"] = "image_url";
                            c["image_url"] = image_url;
                        } else {
                            std::cerr << "图像文件base64编码失败: " << content.image_url << std::endl;
                            c["type"] = "text";
                            c["text"] = "[图像文件不可用: " + content.image_url + "]";
                        }
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
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "X-DashScope-SSE: disable");
        
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
            response.content = "Error: Failed to call Qwen API: " + std::string(curl_easy_strerror(res));
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
                    if (response_json.isMember("choices") && response_json["choices"].isArray() && 
                        response_json["choices"].size() > 0) {
                        response.content = response_json["choices"][0]["message"]["content"].asString();
                    } else if (response_json.isMember("error") && response_json["error"].isMember("message")) {
                        response.content = "Error: " + response_json["error"]["message"].asString();
                    } else {
                        response.content = "Error: Invalid response format";
                    }
                } else {
                    response.content = "Error: Failed to parse API response: " + errs;
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
        // 构建请求URL
        std::string url = base_url_ + "/compatible-mode/v1/embeddings";
        
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
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
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
            response.message = "Error: Failed to call Qwen Embedding API: " + std::string(curl_easy_strerror(res));
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
                if (response_json.isMember("data") && response_json["data"].isArray()) {
                    for (const auto& item : response_json["data"]) {
                        QwenEmbeddingData data;
                        data.index = item["index"].asInt();
                        
                        Json::Value embedding = item["embedding"];
                        if (embedding.isArray()) {
                            for (const auto& val : embedding) {
                                data.embedding.push_back(val.asFloat());
                            }
                        }
                        
                        response.data.push_back(data);
                    }
                } else if (response_json.isMember("error") && response_json["error"].isMember("message")) {
                    response.message = "Error: " + response_json["error"]["message"].asString();
                } else {
                    response.message = "Error: Invalid response format";
                }
            } else {
                response.message = "Error: Failed to parse API response: " + errs;
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