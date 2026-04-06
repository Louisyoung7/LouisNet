#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "base/noncopyable.h"

namespace net::reactor {
class Channel;
class Poller;

// EventLoop类
// 1.管理EventLoop内部的Poller实例
// 2.管理活跃的Channel列表
// 3.提供事件循环的启动和退出接口
// 4.提供Channel的添加和移除接口

class EventLoop : public base::noncopyable {
    struct Impl;                  // 定义内部结构体
    std::unique_ptr<Impl> impl_;  // 指向内部结构体实例的指针

   public:
    using Functor = std::function<void()>;
    using ChannelList = std::vector<Channel*>;

    EventLoop();
    ~EventLoop();

    // 运行事件循环
    void loop();

    // 退出事件循环
    void quit();

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

    // 确保回调在loop线程执行
    void runInLoop(Functor cb);

    // 将回调放入任务队列，并唤醒loop线程
    void queueInLoop(Functor cb);

    // 判断是否在loop线程
    bool isInLoopThread();

   private:
    // 调用Poller的poll，填充活跃的Channel列表
    void poll(int timeout_ms, ChannelList& active_channels);

    // 执行待处理任务
    void doPendingFunctors();

    // 唤醒loop线程
    void wakeup();
};
}  // namespace net::reactor