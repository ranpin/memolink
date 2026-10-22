#pragma once

#include <string>
#include <chrono>

class TimeUtils {
public:
    // 获取当前时间戳字符串
    static std::string getCurrentTimestamp();
    
    // 将时间点格式化为字符串
    static std::string formatTime(const std::chrono::system_clock::time_point& timePoint);
    
    // 获取北京时间的时间戳字符串
    static std::string getBeijingTimestamp();
    
    // 将时间点转换为北京时间字符串
    static std::string timePointToBeijingString(const std::chrono::system_clock::time_point& tp);
};