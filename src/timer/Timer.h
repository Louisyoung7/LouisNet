#pragma once

#include <chrono>
#include <functional>
#include <optional>

#include "base/noncopyable.h"
#include "timer/Timestamp.h"

namespace timer {
class Timer : public base::noncopyable {
   public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    using TimerCallback = std::function<void()>;

    Timer() = default;
    Timer(Timestamp expiration, Duration interval, TimerCallback cb, uint64_t id)
        : expiration_(expiration), cb_(cb), repeat_(interval > Duration(0)), interval_(interval), id_(id) {}

    void run() const { cb_(); }
    void restart(Timestamp now) {
        if (repeat_) {
            expiration_ = (now += interval_);
        } else {
            expiration_ = std::nullopt;
        }
    }

    bool operator<(const Timer& rhs) const { return expiration_ < rhs.expiration_; }
    bool operator=(const Timer& rhs) const { return expiration_ == rhs.expiration_; }
    bool operator>(const Timer& rhs) const { return expiration_ > rhs.expiration_; }

    std::optional<Timestamp> expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    Duration interval() const { return interval_; }
    uint64_t id() const { return id_; }

   private:
    std::optional<Timestamp> expiration_;
    TimerCallback cb_;
    bool repeat_;
    Duration interval_;
    uint64_t id_;
};
}  // namespace timer