#include <gtest/gtest.h>
#include <sys/socket.h>

#include "net/reactor/Channel.h"
#include "net/reactor/EventLoop.h"
#include "net/reactor/Poller.h"

class PollerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // 创建EventLoop，EventLoop会创建Poller
        loop_ = new net::reactor::EventLoop();
    }

    void TearDown() override {
        // 清理资源
        delete loop_;
    }

    net::reactor::EventLoop* loop_;
};

// 测试管理多个Channel
TEST_F(PollerTest, MutipleChannels) {
    // 创建多个Channel
    int sockfd1 = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    ASSERT_GE(sockfd1, 0);
    net::reactor::Channel* channel1 = new net::reactor::Channel(loop_, sockfd1);

    int sockfd2 = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    ASSERT_GE(sockfd2, 0);
    net::reactor::Channel* channel2 = new net::reactor::Channel(loop_, sockfd2);

    // 测试启用读事件
    channel1->enableRead();
    EXPECT_EQ(channel1->index(), net::reactor::Channel::kAdded);
    channel2->enableRead();
    EXPECT_EQ(channel2->index(), net::reactor::Channel::kAdded);

    // 测试禁用所有事件
    channel1->disableAll();
    EXPECT_EQ(channel1->index(), net::reactor::Channel::kDeleted);
    channel2->disableAll();
    EXPECT_EQ(channel2->index(), net::reactor::Channel::kDeleted);

    // 测试删除多个Channel
    channel1->remove();
    EXPECT_EQ(channel1->index(), net::reactor::Channel::kNew);
    channel2->remove();
    EXPECT_EQ(channel2->index(), net::reactor::Channel::kNew);

    delete channel1;
    delete channel2;
}