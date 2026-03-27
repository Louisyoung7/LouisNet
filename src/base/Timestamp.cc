#include "Timestamp.h"

#include <chrono>

namespace base {
Timestamp Timestamp::now() {
    auto now = std::chrono::system_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    return Timestamp(micros);
}

std::string Timestamp::toString() const {
    char buf[128]{};
    // 获取时间点对象
    auto now = std::chrono::system_clock::time_point() + std::chrono::microseconds(microSecondsSinceEpoch_);
    // 转换为time_t对象
    auto time_t_value = std::chrono::system_clock::to_time_t(now);

    // 转换为tm对象
    tm tm_time;
    // 使用线程安全的localtime_r方法
    localtime_r(&time_t_value, &tm_time);

    // 格式化时间字符串
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    return buf;
}
}  // namespace base