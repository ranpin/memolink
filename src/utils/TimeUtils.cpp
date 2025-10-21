#include "TimeUtils.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

std::string TimeUtils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    return timePointToBeijingString(now);
}

std::string TimeUtils::formatTime(const std::chrono::system_clock::time_point& timePoint) {
    return timePointToBeijingString(timePoint);
}

std::string TimeUtils::getBeijingTimestamp() {
    auto now = std::chrono::system_clock::now();
    return timePointToBeijingString(now);
}

std::string TimeUtils::timePointToBeijingString(const std::chrono::system_clock::time_point& tp) {
    // 转换为time_t
    std::time_t utc_time = std::chrono::system_clock::to_time_t(tp);
    
    // 获取本地时间（系统应该已经设置为正确的时区）
    std::tm* local_tm = std::localtime(&utc_time);
    
    std::stringstream ss;
    ss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}