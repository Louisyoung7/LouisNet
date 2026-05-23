#pragma once

#include <chrono>
#include <utility>

namespace timer {
class Timestamp {
   public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    // 默认构造为 epoch (1970-01-01 00:00:00 UTC)
    Timestamp() : timePoint_(TimePoint()) {}

    // 从time_point构造
    explicit Timestamp(TimePoint timePoint) : timePoint_(std::move(timePoint)) {}

    static Timestamp now() { return Timestamp(Clock::now()); }
    static Timestamp epoch() { return Timestamp(); }

    Timestamp operator+=(Duration duration) {
        timePoint_ += duration;
        return *this;
    }
    Timestamp operator-=(Duration duration) {
        timePoint_ -= duration;
        return *this;
    }

    Duration operator-(const Timestamp& rhs) const { return timePoint_ - rhs.timePoint_; }
    

    bool operator>(const Timestamp& rhs) const { return timePoint_ > rhs.timePoint_; }
    bool operator<(const Timestamp& rhs) const { return timePoint_ < rhs.timePoint_; }
    bool operator==(const Timestamp& rhs) const { return timePoint_ == rhs.timePoint_; }

   private:
    TimePoint timePoint_;
};
}  // namespace timer
