#pragma once

#include <chrono>
#include <functional>

namespace timer {
class Timer {
   public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Microseconds = std::chrono::microseconds;
    using Nanoseconds = std::chrono::nanoseconds;

    using Callback = std::function<void()>;

    Timer() = default;
    Timer(TimePoint expiration, Nanoseconds interval, Callback cb)
        : expiration_(expiration), cb_(cb), repeat_(interval > Nanoseconds(0)), interval_(interval) {}

    bool operator<(const Timer& rhs) const { return expiration_ < rhs.expiration_; }
    bool operator=(const Timer& rhs) const { return expiration_ == rhs.expiration_; }
    bool operator>(const Timer& rhs) const { return expiration_ > rhs.expiration_; }

    Nanoseconds interval() const { return interval_; }

   private:
    TimePoint expiration_;
    Callback cb_;
    bool repeat_;
    Nanoseconds interval_;
};
}  // namespace timer