#include <gtest/gtest.h>

#include "timer/Timer.h"

namespace {

using timer::Timer;
using timer::Timestamp;

class TimerTest : public ::testing::Test {
   protected:
    void SetUp() override {}
};

TEST_F(TimerTest, DefaultConstruction) {
    Timer timer;
    EXPECT_FALSE(timer.expiration().has_value());
    EXPECT_FALSE(timer.repeat());
    EXPECT_EQ(timer.interval(), timer::Duration(0));
    EXPECT_EQ(timer.id(), 0u);
}

TEST_F(TimerTest, ConstructionWithParameters) {
    Timestamp expiration = Timestamp::now();
    auto interval = std::chrono::seconds(5);
    bool callback_invoked = false;
    auto cb = [&callback_invoked]() { callback_invoked = true; };

    Timer timer(expiration, interval, cb, 42);

    EXPECT_TRUE(timer.expiration().has_value());
    EXPECT_EQ(timer.expiration().value(), expiration);
    EXPECT_TRUE(timer.repeat());
    EXPECT_EQ(timer.interval(), interval);
    EXPECT_EQ(timer.id(), 42u);
}

TEST_F(TimerTest, NonRepeatingTimer) {
    Timestamp expiration = Timestamp::now();
    auto interval = timer::Duration(0);
    auto cb = []() {};

    Timer timer(expiration, interval, cb, 1);
    EXPECT_FALSE(timer.repeat());
}

TEST_F(TimerTest, Run) {
    bool callback_invoked = false;
    auto cb = [&callback_invoked]() { callback_invoked = true; };

    Timer timer(Timestamp::now(), std::chrono::seconds(1), cb, 1);
    EXPECT_FALSE(callback_invoked);

    timer.run();
    EXPECT_TRUE(callback_invoked);
}

TEST_F(TimerTest, RestartRepeatingTimer) {
    auto interval = std::chrono::seconds(5);
    auto cb = []() {};
    Timestamp start = Timestamp::epoch();

    Timer timer(start, interval, cb, 1);
    EXPECT_TRUE(timer.repeat());

    Timestamp now = start + std::chrono::seconds(1);

    timer.restart(now);

    EXPECT_TRUE(timer.expiration().has_value());
    EXPECT_EQ(timer.expiration().value(), now + interval);
}

TEST_F(TimerTest, RestartNonRepeatingTimer) {
    auto interval = timer::Duration(0);
    auto cb = []() {};

    Timer timer(Timestamp::epoch(), interval, cb, 1);
    EXPECT_FALSE(timer.repeat());

    timer.restart(Timestamp::now());

    EXPECT_FALSE(timer.expiration().has_value());
}

TEST_F(TimerTest, ComparisonOperators) {
    Timestamp t1 = Timestamp::epoch();
    Timestamp t2 = t1 + std::chrono::seconds(10);
    Timestamp t3 = t2 + std::chrono::seconds(10);

    Timer timer1(t1, std::chrono::seconds(1), []() {}, 1);
    Timer timer2(t2, std::chrono::seconds(1), []() {}, 2);
    Timer timer3(t3, std::chrono::seconds(1), []() {}, 3);

    EXPECT_TRUE(timer1 < timer2);
    EXPECT_TRUE(timer2 < timer3);
    EXPECT_TRUE(timer3 > timer1);
    EXPECT_FALSE(timer1 > timer2);
}

TEST_F(TimerTest, EqualityOperator) {
    Timestamp t1 = Timestamp::epoch();
    Timestamp t2 = t1 + std::chrono::seconds(5);

    Timer timer1(t1, std::chrono::seconds(1), []() {}, 1);
    Timer timer2(t1, std::chrono::seconds(2), []() {}, 2);
    Timer timer3(t2, std::chrono::seconds(1), []() {}, 3);

    EXPECT_TRUE(timer1 == timer2);
    EXPECT_FALSE(timer1 == timer3);
}

}  // namespace
