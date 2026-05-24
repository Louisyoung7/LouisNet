#include <iostream>

#include "log/Logger.h"
#include "net/reactor/EventLoop.h"
#include "timer/Timer.h"

using namespace net::reactor;
using namespace timer;

int main() {
    init();                                        // 先初始化日志系统
    setLogLevel(spdlog::level::level_enum::info);  // 设置日志级别

    EventLoop loop;  // EventLoop 构造，在日志系统初始化之后调用

    info("[EventLoop] started.\n\n");

    loop.runAfter(timer::Duration(std::chrono::seconds(2)),
                  timer::TimerCallback([] { std::cout << "Hello, World!" << std::endl; }));

    loop.runAt(timer::Timestamp::now() + timer::Duration(std::chrono::seconds(2)),
               timer::TimerCallback([] { std::cout << "Hello, World!" << std::endl; }));

    auto timerId = loop.runEvery(timer::Duration(std::chrono::seconds(2)),
                                 timer::TimerCallback([] { std::cout << "Hello, World!" << std::endl; }));

    loop.runAfter(timer::Duration(std::chrono::seconds(8)), [timerId, &loop]() { loop.cancel(timerId); });

    loop.runAfter(timer::Duration(std::chrono::seconds(10)), [&loop]() { loop.quit(); });

    loop.loop();

    shutdown();

    return 0;
}
