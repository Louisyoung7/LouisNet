#pragma once

#include <sys/timerfd.h>
#include <unistd.h>

#include <atomic>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/noncopyable.h"
#include "net/reactor/Channel.h"
#include "timer/Timer.h"
#include "timer/TimerId.h"

namespace net::reactor {
class EventLoop;
}

namespace timer {
class TimerScheduler : public base::noncopyable {
   public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    using TimerCallback = std::function<void()>;

    TimerScheduler() = default;
    explicit TimerScheduler(net::reactor::EventLoop* loop);
    ~TimerScheduler();

    // 添加定时器，返回定时器ID
    TimerId addTimer(Timestamp expiration, Duration interval, TimerCallback cb);
    // 取消定时器
    void cancelTimer(TimerId timerId);

   private:
    using Entry = std::pair<Timestamp, std::shared_ptr<Timer>>;
    using TimerSet = std::set<Entry>;
    using TimerIdMap = std::unordered_map<uint64_t, std::weak_ptr<Timer>>;

    net::reactor::EventLoop* loop_;
    int timerfd_;
    net::reactor::Channel timerChannel_;
    TimerSet timers_;
    TimerIdMap activeTimers_;
    TimerIdMap cancelingTimers_;
    std::atomic<uint64_t> nextId_{0};
    bool callingExpiredTimers_{false};

    void addTimerInLoop(uint64_t id, std::shared_ptr<Timer> timer);
    void cancelTimerInLoop(TimerId timerId);

    void handleRead();

    // 提取所有到期的定时器
    std::vector<Entry> extractExpiredTimers(Timestamp now);
    // 重置到期的重复定时器，删除非重复定时器
    void reset(const std::vector<Entry>& expiredTimers, Timestamp now);
};
}  // namespace timer