#include <gtest/gtest.h>

#include "timer/Timestamp.h"

namespace {

using timer::Timestamp;

class TimestampTest : public ::testing::Test {
   protected:
    void SetUp() override {}
};

TEST_F(TimestampTest, DefaultConstruction) {
    Timestamp ts;
    EXPECT_EQ(ts, Timestamp::epoch());
}

TEST_F(TimestampTest, NowIsAfterEpoch) {
    Timestamp now = Timestamp::now();
    Timestamp epoch = Timestamp::epoch();
    EXPECT_GT(now, epoch);
}

TEST_F(TimestampTest, EpochReturnsDefaultTimestamp) { EXPECT_EQ(Timestamp::epoch(), Timestamp()); }

TEST_F(TimestampTest, AdditionAssignment) {
    Timestamp ts = Timestamp::epoch();
    auto duration = std::chrono::seconds(5);
    ts += duration;
    EXPECT_GT(ts, Timestamp::epoch());
}

TEST_F(TimestampTest, SubtractionAssignment) {
    Timestamp ts = Timestamp::now();
    auto duration = std::chrono::seconds(1);
    ts -= duration;
    EXPECT_LT(ts, Timestamp::now());
}

TEST_F(TimestampTest, SubtractionReturnsDuration) {
    Timestamp t1 = Timestamp::epoch();
    Timestamp t2 = Timestamp::epoch() += std::chrono::seconds(10);
    auto duration = t2 - t1;
    EXPECT_EQ(duration, std::chrono::seconds(10));
}

TEST_F(TimestampTest, ComparisonOperators) {
    Timestamp t1 = Timestamp::epoch();
    Timestamp t2 = Timestamp::epoch() += std::chrono::seconds(1);

    EXPECT_TRUE(t2 > t1);
    EXPECT_TRUE(t1 < t2);
    EXPECT_FALSE(t1 > t2);
    EXPECT_FALSE(t2 < t1);
}

TEST_F(TimestampTest, EqualityOperator) {
    Timestamp t1 = Timestamp::now();
    Timestamp t2 = Timestamp::epoch();
    Timestamp t3 = Timestamp::epoch();

    EXPECT_TRUE(t1 == t1);
    EXPECT_TRUE(t2 == t3);
    EXPECT_FALSE(t1 == t2);
}

}  // namespace
