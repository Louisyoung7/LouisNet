#pragma once

#include <sys/timerfd.h>
#include <unistd.h>

#include <memory>
#include <mutex>
#include <set>

#include "base/noncopyable.h"
#include "net/reactor/Channel.h"
#include "timer/Timer.h"

namespace net::reactor {
class EventLoop;
}

namespace timer {
class TimerQueue : public base::noncopyable {
   public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    using TimerCallback = std::function<void()>;

    TimerQueue() = default;

    explicit TimerQueue(net::reactor::EventLoop* loop)
        : loop_(loop),
          timerfd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
          timerChannel_(loop, timerfd_) {
        timerChannel_.setReadCallback([this]() { handleRead(); });
        timerChannel_.enableRead();
    }
    ~TimerQueue() { ::close(timerfd_); }

    void addTimer(Timer timer);

   private:
    using Entry = std::pair<Timestamp, std::unique_ptr<Timer>>;
    using TimerSet = std::set<Entry>;

    net::reactor::EventLoop* loop_;
    int timerfd_;
    net::reactor::Channel timerChannel_;
    TimerSet timers_;
    std::mutex mutex_;

    void handleRead();
};
}  // namespace timer