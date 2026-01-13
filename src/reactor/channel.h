#pragma once

#include <sys/epoll.h>  // for epoll_event

#include <cstdint>  // for uint32_t
#include <functional>

namespace reactor {
class EventLoop;
class Poller;

class Channel {
   public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void()>;

    static constexpr int kNoneEvent_ = 0;
    static constexpr int kReadEvent_ = EPOLLIN;
    static constexpr int kWriteEvent_ = EPOLLOUT;
    static constexpr int kCloseEvent = EPOLLHUP | EPOLLERR;
    static constexpr int kErrorEvent = EPOLLERR;

    // 构造析构
    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 获取套接字
    int fd() const;

    // 获取关心的事件
    uint32_t events() const;

    // 设置发生的事件
    void setRevents(uint32_t revents);

    // 设置回调函数
    void setReadCallback(const EventCallback& callback);
    void setWriteCallback(const EventCallback& callback);

    // 注册，更新事件
    void enableRead();
    void disableRead();
    void enableWrite();
    void disableWrite();
    void disableAll();

    // 处理事件
    void handleEvent();

   private:
    EventLoop* loop_;                 // 所属的EventLoop
    const int fd_;                    // 封装的套接字
    uint32_t events_ = kNoneEvent_;   // 关心的事件
    uint32_t revents_ = kNoneEvent_;  // 实际发生的事件

    // 回调函数
    EventCallback readCallback_;
    EventCallback writeCallback_;

   private:
    // 更新Channel
    void update();
};
}  // namespace reactor