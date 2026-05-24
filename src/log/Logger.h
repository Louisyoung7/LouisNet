#pragma once

#include "spdlog/spdlog.h"

// 初始化日志系统
void init();

// 跟踪日志
template <typename... Args>
void trace(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}
// 调试日志
template <typename... Args>
void debug(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}
// 信息日志
template <typename... Args>
void info(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}
// 警告日志
template <typename... Args>
void warn(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}
// 错误日志
template <typename... Args>
void error(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}
// 严重错误日志
template <typename... Args>
void critical(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

inline void shutdown() {
    spdlog::drop_all();
    spdlog::shutdown();
}

inline void setLogLevel(spdlog::level::level_enum level) { spdlog::set_level(level); }
