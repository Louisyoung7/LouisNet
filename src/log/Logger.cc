#include "log/Logger.h"

#include <memory>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using namespace spdlog;

static std::shared_ptr<logger> logger_;

void init() {
    init_thread_pool(8192, 2);

    auto console_sink = std::make_shared<sinks::stdout_color_sink_mt>();
    console_sink->set_level(level::trace);  // 设置 sink 级别为 trace

    logger_ = std::make_shared<async_logger>("server", console_sink, thread_pool(),
                                             spdlog::async_overflow_policy::discard_new);
    logger_->set_level(level::trace);  // 设置 logger 级别为 trace
    set_default_logger(logger_);
}
