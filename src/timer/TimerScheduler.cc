#include "TimerScheduler.h"

#include <cassert>

#include "log/Logger.h"
#include "net/reactor/EventLoop.h"

using namespace timer;

namespace {
// 读取timerfd，清除读标志
void readTimerfd(int timerfd) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    if (n != sizeof(howmany)) { logging::error("[TimerScheduler] readTimerfd() reads {} bytes instead of 8.\n\n", n); }
}

// 重置timerfd，设置到期时间
void resetTimerfd(int timerfd, Timestamp expiration) {
    struct itimerspec spec {};

    auto duration = expiration - Timestamp::now();
    spec.it_value.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    spec.it_value.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;

    spec.it_interval = spec.it_value;

    ::timerfd_settime(timerfd, 0, &spec, nullptr);
}
}  // namespace

TimerScheduler::TimerScheduler(net::reactor::EventLoop* loop)
    : loop_(loop),
      timerfd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
      timerChannel_(loop, timerfd_) {
    timerChannel_.setReadCallback([this]() { handleRead(); });
    timerChannel_.enableRead();
}

TimerScheduler::~TimerScheduler() {
    timerChannel_.disableAll();
    timerChannel_.remove();
    ::close(timerfd_);
}

// 添加定时器，返回定时器ID
TimerId TimerScheduler::addTimer(Timestamp expiration, Duration interval, TimerCallback cb) {
    // 创建定时器对象，生成唯一ID，并添加到EventLoop线程中执行
    std::shared_ptr<Timer> timer = std::make_shared<Timer>(expiration, interval, std::move(cb), nextId_.load() + 1);
    nextId_.store(timer->id());
    addTimerInLoop(timer->id(), timer);

    // 返回定时器ID
    return TimerId(timer->id(), timer);
}

// 取消定时器
void TimerScheduler::cancelTimer(TimerId timerId) {
    loop_->runInLoop([this, timerId]() { cancelTimerInLoop(timerId); });
}

// 在EventLoop线程中执行添加定时器操作
void TimerScheduler::addTimerInLoop(uint64_t id, std::shared_ptr<Timer> timer) {
    assert(loop_->isInLoopThread());
    assert(timers_.size() == activeTimers_.size());

    // 检查是否需要更新earliestChanged标志，如果新定时器的到期时间早于当前最早的定时器，则需要重置timerfd到期时间
    bool earliestChanged = false;
    auto expiration = timer->expiration();
    auto it = timers_.begin();
    if (it == timers_.end() || (expiration && (expiration < it->first))) { earliestChanged = true; }

    {
        [[maybe_unused]] auto [it, inserted] = timers_.emplace(expiration.value(), timer);
        assert(inserted);
    }

    {
        [[maybe_unused]] auto [it, inserted] = activeTimers_.emplace(id, timer);
        assert(inserted);
    }

    assert(timers_.size() == activeTimers_.size());

    if (earliestChanged) {
        // 重置timerfd到期时间
        auto expiration = timer->expiration();
        if (expiration) { resetTimerfd(timerfd_, expiration.value()); }
    }
}

// 在EventLoop线程中执行取消定时器操作
void TimerScheduler::cancelTimerInLoop(TimerId timerId) {
    // 从activeTimers_中查找
    auto it = activeTimers_.find(timerId.id());
    if (it != activeTimers_.end()) {
        assert(timerId.timer() == it->second);
        auto timer = it->second.lock();
        if (timer) {
            // 从timers_和activeTimers_中删除定时器
            timers_.erase(Entry(timer->expiration().value(), timer));
            activeTimers_.erase(it);
        }
    } else if (callingExpiredTimers_) {
        // 正在执行到期定时器回调，将定时器ID添加到cancelingTimers_中
        cancelingTimers_.emplace(timerId.id(), timerId.timer());
    }

    // 既不在activeTimers_中，也不在cancelingTimers_中，说明定时器已取消或已执行完毕，无需处理
}

void TimerScheduler::handleRead() {
    assert(loop_->isInLoopThread());

    // 读取timerfd，清除读标志
    readTimerfd(timerfd_);

    // 提取所有到期的定时器
    Timestamp now = Timestamp::now();
    auto expiredTimers = extractExpiredTimers(now);

    callingExpiredTimers_ = true;

    // 清空cancelingTimers_
    cancelingTimers_.clear();
    // 执行所有到期的定时器回调
    for (const auto& entry : expiredTimers) { entry.second->run(); }

    callingExpiredTimers_ = false;

    // 重置到期的重复定时器，删除非重复定时器
    reset(expiredTimers, now);
}

// 提取所有到期的定时器
std::vector<TimerScheduler::Entry> TimerScheduler::extractExpiredTimers(Timestamp now) {
    assert(timers_.size() == activeTimers_.size());

    std::vector<Entry> expiredTimers;

    // upper_bound返回第一个大于指定值的元素的迭代器
    auto it = timers_.upper_bound(Entry(now, nullptr));
    assert(it == timers_.end() || now < it->first);

    // 复制所有到期的定时器到expiredTimers
    std::copy(timers_.begin(), it, std::back_inserter(expiredTimers));

    // 从timers_中删除所有到期的定时器
    timers_.erase(timers_.begin(), it);

    // 从activeTimers_中删除所有到期的定时器
    for (const auto& entry : expiredTimers) {
        auto timer = entry.second;
        auto id = timer->id();
        auto it = activeTimers_.find(id);
        if (it != activeTimers_.end()) { activeTimers_.erase(it); }
    }

    assert(timers_.size() == activeTimers_.size());

    return expiredTimers;
}

// 重置到期的重复定时器，删除非重复定时器
void TimerScheduler::reset(const std::vector<TimerScheduler::Entry>& expiredTimers, Timestamp now) {
    for (const auto& entry : expiredTimers) {
        auto timer = entry.second;
        if (timer->repeat() && cancelingTimers_.find(timer->id()) == cancelingTimers_.end()) {
            timer->restart(now);
            addTimerInLoop(timer->id(), timer);
        }
    }
}
