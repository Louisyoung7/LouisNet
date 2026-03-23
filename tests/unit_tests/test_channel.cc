#include <gtest/gtest.h>
#include <sys/socket.h>

#include "net/reactor/Channel.h"
#include "net/reactor/EventLoop.h"

class ChannelTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // 创建EventLoop
        loop_ = new net::reactor::EventLoop();

        // 创建非阻塞的socket用于测试
        int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        ASSERT_GE(sockfd, 0);

        // 创建Channel
        channel_ = new net::reactor::Channel(loop_, sockfd);
    }

    void TearDown() override {
        // 清理资源
        delete channel_;
        delete loop_;
    }

    net::reactor::EventLoop* loop_;
    net::reactor::Channel* channel_;
};

// 测试Channel的构造和基本属性
TEST_F(ChannelTest, ConstructorAndBasicProperties) {
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kNew);
    EXPECT_EQ(channel_->ownerLoop(), loop_);
    EXPECT_TRUE(channel_->isNoneEvent());
    EXPECT_FALSE(channel_->isReading());
    EXPECT_FALSE(channel_->isWriting());
}

// 测试事件的开启和关闭以及状态管理
TEST_F(ChannelTest, EnableDisableEventsAndStateManagement) {
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kNew);

    // 测试开启读事件
    channel_->enableRead();
    EXPECT_TRUE(channel_->isReading());
    EXPECT_FALSE(channel_->isWriting());
    EXPECT_FALSE(channel_->isNoneEvent());
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kAdded);

    // 测试开启写事件
    channel_->enableWrite();
    EXPECT_TRUE(channel_->isReading());
    EXPECT_TRUE(channel_->isWriting());
    EXPECT_FALSE(channel_->isNoneEvent());
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kAdded);

    // 测试关闭读事件
    channel_->disableRead();
    EXPECT_FALSE(channel_->isReading());
    EXPECT_TRUE(channel_->isWriting());
    EXPECT_FALSE(channel_->isNoneEvent());
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kAdded);

    // 测试关闭写事件
    channel_->disableWrite();
    EXPECT_FALSE(channel_->isReading());
    EXPECT_FALSE(channel_->isWriting());
    EXPECT_TRUE(channel_->isNoneEvent());
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kDeleted);

    // 测试关闭所有事件
    channel_->enableRead();
    channel_->enableWrite();
    EXPECT_TRUE(channel_->isReading());
    EXPECT_TRUE(channel_->isWriting());
    channel_->disableAll();
    EXPECT_FALSE(channel_->isReading());
    EXPECT_FALSE(channel_->isWriting());
    EXPECT_TRUE(channel_->isNoneEvent());
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kDeleted);

    // 将Channel从EventLoop移除
    channel_->remove();
    EXPECT_EQ(channel_->index(), net::reactor::Channel::kNew);
}

// 测试事件回调设置函数和事件处理函数
TEST_F(ChannelTest, SetCallbacksAndHandleEvents) {
    bool readCalled = false;
    bool writeCalled = false;
    bool closeCalled = false;
    bool errorCalled = false;

    // 设置回调
    channel_->setReadCallback([&]() { readCalled = true; });
    channel_->setWriteCallback([&]() { writeCalled = true; });
    channel_->setCloseCallback([&]() { closeCalled = true; });
    channel_->setErrorCallback([&]() { errorCalled = true; });

    // 测试读事件
    channel_->setRevents(net::reactor::Channel::kReadEvent);
    channel_->handleEvent();
    EXPECT_TRUE(readCalled);
    EXPECT_FALSE(writeCalled);
    EXPECT_FALSE(closeCalled);
    EXPECT_FALSE(errorCalled);

    // 重置标志
    readCalled = false;
    writeCalled = false;
    closeCalled = false;
    errorCalled = false;

    // 测试写事件
    channel_->setRevents(net::reactor::Channel::kWriteEvent);
    channel_->handleEvent();
    EXPECT_FALSE(readCalled);
    EXPECT_TRUE(writeCalled);
    EXPECT_FALSE(closeCalled);
    EXPECT_FALSE(errorCalled);

    // 重置标志
    readCalled = false;
    writeCalled = false;
    closeCalled = false;
    errorCalled = false;

    // 测试关闭事件
    channel_->setRevents(net::reactor::Channel::kCloseEvent);
    channel_->handleEvent();
    EXPECT_FALSE(readCalled);
    EXPECT_FALSE(writeCalled);
    EXPECT_TRUE(closeCalled);
    EXPECT_FALSE(errorCalled);

    // 重置标志
    readCalled = false;
    writeCalled = false;
    closeCalled = false;
    errorCalled = false;

    // 测试错误事件
    channel_->setRevents(net::reactor::Channel::kErrorEvent);
    channel_->handleEvent();
    EXPECT_FALSE(readCalled);
    EXPECT_FALSE(writeCalled);
    EXPECT_FALSE(closeCalled);
    EXPECT_TRUE(errorCalled);

    // 重置标志
    readCalled = false;
    writeCalled = false;
    closeCalled = false;
    errorCalled = false;
}