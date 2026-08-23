#include "log/Logger.h"

#include <memory>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using namespace spdlog;

static std::shared_ptr<logger> logger_;

void init() {
    init_thread_pool(8192, 2);

#ifdef NDEBUG
    // Release/RelWithDebInfo/MinSizeRel 模式：仅保留 info 及以上级别
    const auto log_level = level::info;
#else
    // Debug 模式：保留所有日志
    const auto log_level = level::trace;
#endif

    auto console_sink = std::make_shared<sinks::stdout_color_sink_mt>();
    console_sink->set_level(log_level);

    logger_ = std::make_shared<async_logger>("server", console_sink, thread_pool(),
                                             spdlog::async_overflow_policy::discard_new);
    logger_->set_level(log_level);
    set_default_logger(logger_);
}
