#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace net::reactor {
class Channel;
class Poller;

class EventLoop {
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

   private:
    // 调用Poller的poll，填充活跃的Channel列表
    void poll(int timeout_ms, ChannelList& active_channels);

    struct Impl;                  // 定义内部结构体
    std::unique_ptr<Impl> impl_;  // 指向内部结构体实例的指针
};
}  // namespace net::reactor