#include "log/Logger.h"

#include <memory>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace spdlog;

static std::shared_ptr<logger> logger_;

void init() {
    init_thread_pool(8192, 2);

    auto console_sink = std::make_shared<sinks::stdout_color_sink_mt>();
    console_sink->set_level(level::info);

    logger_ = std::make_shared<async_logger>("server", console_sink, thread_pool(),
                                             spdlog::async_overflow_policy::discard_new);
    set_default_logger(logger_);
}
