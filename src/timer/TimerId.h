#pragma once

#include <cstdint>
#include <memory>

#include "Timer.h"

namespace timer {
class TimerId {
   public:
    TimerId() = default;
    explicit TimerId(uint64_t id, std::weak_ptr<Timer> timer) : id_(id), timer_(timer) {}
    ~TimerId() = default;

    uint64_t id() const { return id_; }
    std::weak_ptr<Timer> timer() const { return timer_; }

    bool operator==(const TimerId& other) const { return id_ == other.id_; }
    bool operator!=(const TimerId& other) const { return id_ != other.id_; }

   private:
    friend class Timer;

    uint64_t id_;
    std::weak_ptr<Timer> timer_;
};
}  // namespace timer
