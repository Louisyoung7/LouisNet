#pragma once

#include <chrono>
#include <utility>

namespace timer {
class Timestamp {
   public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Microseconds = std::chrono::microseconds;
    using Nanoseconds = std::chrono::nanoseconds;

    // 默认构造为 epoch (1970-01-01 00:00:00 UTC)
    Timestamp() : timePoint_(TimePoint()) {}

    // 从time_point构造
    explicit Timestamp(TimePoint timePoint) : timePoint_(std::move(timePoint)) {}

    static Timestamp now() { return Timestamp(Clock::now()); }
    static Timestamp epoch() { return Timestamp(); }

    Timestamp& operator+=(Nanoseconds ns) {
        timePoint_ += ns;
        return *this;
    }
    Timestamp& operator-=(Nanoseconds ns) {
        timePoint_ -= ns;
        return *this;
    }

   private:
    TimePoint timePoint_;
};
}  // namespace timer
