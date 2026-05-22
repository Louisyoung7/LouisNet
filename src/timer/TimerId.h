#pragma once

#include <cstdint>

namespace timer {
class TimerId {
   public:
    TimerId() = default;
    explicit TimerId(uint64_t id) : id_(id) {}
    ~TimerId() = default;

    uint64_t id() const { return id_; }

    bool operator==(const TimerId& other) const { return id_ == other.id_; }
    bool operator!=(const TimerId& other) const { return id_ != other.id_; }

   private:
    uint64_t id_;
};
}  // namespace timer
