#include "EventLoop.h"

#include <cassert>
#include <memory>

#include "Channel.h"
#include "Poller.h"
#include "base/LouisLog.h"

namespace net::reactor {
// 定义内部结构体
struct EventLoop::Impl {
    std::unique_ptr<Poller> poller;  // 指向EventLoop内部的Poller实例
    ChannelList active_channels;     // 活跃Channel列表
    bool looping = false;            // 是否正在循环
    bool quit = false;               // 是否停止事件循环

    // 构造函数
    explicit Impl(EventLoop* loop) : poller(std::make_unique<Poller>(loop)) {
    }
};

// 构造析构
EventLoop::EventLoop() : impl_(std::make_unique<Impl>(this)) {
}
EventLoop::~EventLoop() = default;

// 运行事件循环
void EventLoop::loop() {
    assert(!impl_->looping);

    impl_->looping = true;
    impl_->quit = false;

    INFO("[EventLoop] loop() started.\n\n");

    while (!impl_->quit) {
        // 填充活跃的Channel列表
        poll(4000, impl_->active_channels);
        // 遍历活跃的Channel列表
        for (auto& channel : impl_->active_channels) {
            INFO_F("[EventLoop] loop() handling event for fd %d.\n\n", channel->fd());
            channel->handleEvent();
        }
        // 清空活跃的Channel列表
        impl_->active_channels.clear();
    }

    INFO("[EventLoop] loop() exited.\n\n");
    impl_->looping = false;
}

// 退出事件循环
void EventLoop::quit() {
    impl_->quit = true;
}

// 更新Channel
void EventLoop::updateChannel(Channel* channel) {
    impl_->poller->updateChannel(channel);
}

// 移除Channel
void EventLoop::removeChannel(Channel* channel) {
    impl_->poller->removeChannel(channel);
}

// 调用Poller的poll
void EventLoop::poll(int timeout_ms, ChannelList& active_channels) {
    impl_->poller->poll(timeout_ms, active_channels);
}
}  // namespace net::reactor