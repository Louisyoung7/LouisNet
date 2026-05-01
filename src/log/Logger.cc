#include "log/Logger.h"

#include <memory>

#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace spdlog;

namespace logging {
static std::shared_ptr<sinks::dist_sink_mt> dist_sink_;
static std::shared_ptr<logger> logger_;

inline void log(level::level_enum level, std::string_view msg) { logger_->log(level, msg); }

void init() {
    init_thread_pool(8192, 2);

    auto console_sink = std::make_shared<sinks::stdout_color_sink_mt>();
    console_sink->set_level(level::info);

    auto file_sink = std::make_shared<sinks::rotating_file_sink_mt>("logs/debug.log", 10 * 1024 * 1024, 5);
    file_sink->set_level(level::debug);

    dist_sink_ = std::make_shared<sinks::dist_sink_mt>();
    dist_sink_->add_sink(console_sink);
    dist_sink_->add_sink(file_sink);
    dist_sink_->set_level(level::debug);

    logger_ =
        std::make_shared<async_logger>("server", dist_sink_, thread_pool(), spdlog::async_overflow_policy::discard_new);
    set_default_logger(logger_);
    logger_->set_level(level::info);
}
}  // namespace logging
