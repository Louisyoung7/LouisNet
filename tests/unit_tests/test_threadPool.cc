#include <gtest/gtest.h>

#include "base/ThreadPool.h"

using namespace base;

class ThreadPoolTest : public ::testing::Test {
   protected:
    void SetUp() override {
        pool_ = std::make_unique<ThreadPool>(4);
    }

    void TearDown() override {
        pool_.reset();
    }

    std::unique_ptr<ThreadPool> pool_;
};

int normalFunction(int x, int y) {
    return x + y;
}

class TestClass {
   public:
    int memberFunction(int x, int y) {
        return x * y;
    }
};

TEST_F(ThreadPoolTest, NormalFunction) {
    auto result = pool_->submit(normalFunction, 10, 20);
    EXPECT_EQ(result.get(), 30);
}

TEST_F(ThreadPoolTest, LambdaExpression) {
    auto result = pool_->submit([](int x, int y) { return x + y; }, 15, 25);
    EXPECT_EQ(result.get(), 40);
}

TEST_F(ThreadPoolTest, MemberFunction) {
    TestClass obj;
    auto result = pool_->submit(&TestClass::memberFunction, &obj, 6, 7);
    EXPECT_EQ(result.get(), 42);
}

TEST_F(ThreadPoolTest, MultipleTasks) {
    std::vector<std::future<int>> results;

    for (int i = 0; i < 10; ++i) {
        results.push_back(pool_->submit([i]() { return i * i; }));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(results[i].get(), i * i);
    }
}
