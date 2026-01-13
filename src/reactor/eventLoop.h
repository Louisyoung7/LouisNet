#pragma once

#include <functional>
#include <memory>

namespace reactor {
class Channel;
class Poller;

class EventLoop {
   public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 运行事件循环
    void loop();

    // 退出事件循环
    void quit();

    // 添加任务到任务队列
    void queueInLoop(Functor task);

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

   private:
    // 执行任务队列的任务
    void doPendingFunctors();

    // 调用Poller的poll
    void poll(int timeout_ms);

    struct Impl;                  // 定义内部结构体
    std::unique_ptr<Impl> impl_;  // 指向内部结构体实例的指针
};
}  // namespace reactor