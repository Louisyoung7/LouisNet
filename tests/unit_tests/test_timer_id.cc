#include <gtest/gtest.h>

#include "timer/Timer.h"
#include "timer/TimerId.h"

namespace {

using timer::Timer;
using timer::TimerId;

class TimerIdTest : public ::testing::Test {
   protected:
    void SetUp() override {}
};

TEST_F(TimerIdTest, DefaultConstruction) {
    TimerId id;
    EXPECT_EQ(id.id(), 0u);
    EXPECT_TRUE(id.timer().expired());
}

TEST_F(TimerIdTest, ConstructionWithIdAndTimer) {
    auto timer = std::make_shared<Timer>();
    TimerId id(42, timer);

    EXPECT_EQ(id.id(), 42u);
    EXPECT_FALSE(id.timer().expired());
    EXPECT_EQ(id.timer().lock(), timer);
}

TEST_F(TimerIdTest, EqualityOperator) {
    auto timer = std::make_shared<Timer>();

    TimerId id1(1, timer);
    TimerId id2(1, timer);
    TimerId id3(2, timer);

    EXPECT_TRUE(id1 == id2);
    EXPECT_FALSE(id1 == id3);
}

TEST_F(TimerIdTest, InequalityOperator) {
    auto timer = std::make_shared<Timer>();

    TimerId id1(1, timer);
    TimerId id2(1, timer);
    TimerId id3(2, timer);

    EXPECT_FALSE(id1 != id2);
    EXPECT_TRUE(id1 != id3);
}

TEST_F(TimerIdTest, TimerWeakPtrExpired) {
    {
        auto timer = std::make_shared<Timer>();
        TimerId id(1, timer);
        EXPECT_FALSE(id.timer().expired());
    }
    TimerId id(1, std::weak_ptr<Timer>{});
    EXPECT_TRUE(id.timer().expired());
}

}  // namespace
