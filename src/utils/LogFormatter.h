#pragma once

#include <string>
#include <iostream>
#include <sstream>

namespace LogFormatter {

// ANSI颜色代码
namespace Color {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    
    // 前景色
    const std::string BLACK = "\033[30m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    
    // 亮色
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";
    
    // 背景色
    const std::string BG_BLACK = "\033[40m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_MAGENTA = "\033[45m";
    const std::string BG_CYAN = "\033[46m";
    const std::string BG_WHITE = "\033[47m";
}

// 边框字符（ASCII兼容）
namespace Box {
    const std::string TOP_LEFT = "+";
    const std::string TOP_RIGHT = "+";
    const std::string BOTTOM_LEFT = "+";
    const std::string BOTTOM_RIGHT = "+";
    const std::string HORIZONTAL = "-";
    const std::string VERTICAL = "|";
    const std::string T_DOWN = "+";
    const std::string T_UP = "+";
    const std::string T_RIGHT = "+";
    const std::string T_LEFT = "+";
    const std::string CROSS = "+";
}

// 打印带颜色的文本
inline void printColored(const std::string& text, const std::string& color = Color::RESET) {
    std::cout << color << text << Color::RESET;
}

// 打印带边框的标题
inline void printTitle(const std::string& title, const std::string& color = Color::BRIGHT_CYAN) {
    size_t width = 80;
    size_t title_len = title.length();
    size_t padding = (width - title_len - 4) / 2;
    
    std::cout << "\n" << color << Color::BOLD;
    std::cout << Box::TOP_LEFT << std::string(width - 2, Box::HORIZONTAL[0]) << Box::TOP_RIGHT << "\n";
    std::cout << Box::VERTICAL << std::string(padding, ' ') << title << std::string(width - padding - title_len - 2, ' ') << Box::VERTICAL << "\n";
    std::cout << Box::BOTTOM_LEFT << std::string(width - 2, Box::HORIZONTAL[0]) << Box::BOTTOM_RIGHT;
    std::cout << Color::RESET << "\n\n";
}

// 打印分隔线
inline void printSeparator(const std::string& color = Color::BRIGHT_BLACK) {
    std::cout << color << std::string(80, '-') << Color::RESET << "\n";
}

// 打印双分隔线
inline void printDoubleSeparator(const std::string& color = Color::BRIGHT_CYAN) {
    std::cout << color << std::string(80, '=') << Color::RESET << "\n";
}

// 打印阶段标题
inline void printStage(const std::string& stage, const std::string& description = "") {
    std::cout << "\n" << Color::BOLD << Color::BRIGHT_YELLOW << "[STEP] " << stage << Color::RESET;
    if (!description.empty()) {
        std::cout << Color::BRIGHT_BLACK << " - " << description << Color::RESET;
    }
    std::cout << "\n";
    printSeparator(Color::YELLOW);
}

// 打印成功消息
inline void printSuccess(const std::string& message) {
    std::cout << Color::BRIGHT_GREEN << "[OK] " << message << Color::RESET << "\n";
}

// 打印错误消息
inline void printError(const std::string& message) {
    std::cout << Color::BRIGHT_RED << "[ERR] " << message << Color::RESET << "\n";
}

// 打印警告消息
inline void printWarning(const std::string& message) {
    std::cout << Color::BRIGHT_YELLOW << "[WARN] " << message << Color::RESET << "\n";
}

// 打印信息消息
inline void printInfo(const std::string& message) {
    std::cout << Color::BRIGHT_BLUE << "[INFO] " << message << Color::RESET << "\n";
}

// 打印带标签的消息
inline void printLabeled(const std::string& label, const std::string& value, 
                        const std::string& labelColor = Color::BRIGHT_CYAN,
                        const std::string& valueColor = Color::WHITE) {
    std::cout << labelColor << label << ": " << Color::RESET 
              << valueColor << value << Color::RESET << "\n";
}

// 打印对话消息（用户）
inline void printUserMessage(const std::string& message) {
    std::cout << "\n" << Color::BRIGHT_CYAN << Color::BOLD << "[用户] > " 
              << Color::RESET << Color::WHITE << message << Color::RESET << "\n";
}

// 打印对话消息（AI）
inline void printAIMessage(const std::string& message) {
    std::cout << "\n" << Color::BRIGHT_MAGENTA << Color::BOLD << "[AI] > " 
              << Color::RESET << Color::WHITE << message << Color::RESET << "\n";
}

// 打印进度
inline void printProgress(const std::string& activity, int current, int total) {
    std::cout << Color::BRIGHT_BLUE << "[BUSY] " << activity << " [" 
              << current << "/" << total << "]" << Color::RESET << "\n";
}

// 打印框式提示
inline void printBox(const std::string& content, const std::string& color = Color::BRIGHT_CYAN) {
    size_t width = 80;
    size_t max_line_width = width - 4; // 留出边框和空格的空间
    
    std::cout << "\n" << color;
    std::cout << Box::TOP_LEFT << std::string(width - 2, Box::HORIZONTAL[0]) << Box::TOP_RIGHT << "\n";
    
    // 分割多行内容
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        // 处理超长行：截断或换行
        if (line.length() > max_line_width) {
            // 将超长行分割成多个短行
            for (size_t pos = 0; pos < line.length(); pos += max_line_width) {
                std::string sub_line = line.substr(pos, max_line_width);
                size_t padding = max_line_width - sub_line.length();
                std::cout << Box::VERTICAL << " " << sub_line << std::string(padding + 1, ' ') << Box::VERTICAL << "\n";
            }
        } else {
            // 正常长度的行
            size_t padding = max_line_width - line.length();
            std::cout << Box::VERTICAL << " " << line << std::string(padding + 1, ' ') << Box::VERTICAL << "\n";
        }
    }
    
    std::cout << Box::BOTTOM_LEFT << std::string(width - 2, Box::HORIZONTAL[0]) << Box::BOTTOM_RIGHT;
    std::cout << Color::RESET << "\n\n";
}

} // namespace LogFormatter
