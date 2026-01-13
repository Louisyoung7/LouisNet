#include "reactor/eventLoop.h"

#include <memory>
#include <vector>

#include "reactor/channel.h"
#include "reactor/poller.h"

namespace reactor {
// 定义内部结构体
struct EventLoop::Impl {
    std::unique_ptr<Poller> poller;         // 指向EventLoop内部的Poller实例
    std::vector<Channel*> active_channels;  // 活跃Channel列表
    std::vector<Functor> pending_functors;  // 任务队列
    bool quit = false;                      // 是否停止事件循环
    bool calling_pending_functors = false;  // 是否在处理任务队列

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
    while (!impl_->quit) {
        // 填充活跃的Channel列表
        poll(4000);
        // 遍历活跃的Channel列表
        for (auto& channel : impl_->active_channels) {
            channel->handleEvent();
        }
        // 清理活跃的Channel列表
        impl_->active_channels.clear();
        // 处理任务队列
        doPendingFunctors();
    }
}

// 退出事件循环
void EventLoop::quit() {
    impl_->quit = true;
}

// 添加任务到任务队列
void EventLoop::queueInLoop(Functor task) {
    impl_->pending_functors.emplace_back(task);
}

// 更新Channel
void EventLoop::updateChannel(Channel* channel) {
    impl_->poller->updateChannel(channel);
}

// 移除Channel
void EventLoop::removeChannel(Channel* channel) {
    impl_->poller->removeChannel(channel);
}

// 执行任务队列的任务
void EventLoop::doPendingFunctors() {
    impl_->calling_pending_functors = true;
    for (const auto& task : impl_->pending_functors) {
        task();
    }
    // 清空任务队列
    impl_->pending_functors.clear();
    impl_->calling_pending_functors = false;
}

// 调用Poller的poll
void EventLoop::poll(int timeout_ms) {
    impl_->poller->poll(timeout_ms, impl_->active_channels);
}
}  // namespace reactor