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

    // 事件类型
    static constexpr int kNoneEvent = 0;                   ///< 不关心任何事件
    static constexpr int kReadEvent = EPOLLIN | EPOLLPRI;  ///< 关心读事件和紧急读事件
    static constexpr int kWriteEvent = EPOLLOUT;           ///< 关心写事件
    static constexpr int kCloseEvent = EPOLLHUP | EPOLLERR;
    static constexpr int kErrorEvent = EPOLLERR;

    //* Channel状态枚举，用于Poller管理
    //* 这些状态是Channel在Poller中的生命周期状态
    static constexpr int kNew = -1;     ///< 未添加到Poller，也未注册到epoll
    static constexpr int kAdded = 0;    ///< 添加到Poller，也注册到epoll
    static constexpr int kDeleted = 1;  ///< 添加到Poller，但未注册到epoll

    // 构造析构
    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 获取套接字
    int fd() const {
        return fd_;
    }
    // 获取关心的事件
    uint32_t events() const {
        return events_;
    }
    // 查看是否没有关心的事件
    bool isNoneEvents() const {
        return events_ == kNoneEvent;
    }
    // 设置发生的事件
    void setRevents(uint32_t revents) {
        revents_ = revents;
    }
    // 获取状态
    int index() const {
        return index_;
    }
    // 设置状态
    void setIndex(int index) {
        index_ = index;
    }

    // 设置回调函数
    void setReadCallback(ReadEventCallback callback) {
        readCallback_ = std::move(callback);
    }
    void setWriteCallback(EventCallback callback) {
        writeCallback_ = std::move(callback);
    }
    void setCloseCallback(EventCallback callback) {
        closeCallback_ = std::move(callback);
    }
    void setErrorCallback(EventCallback callback) {
        errorCallback_ = std::move(callback);
    }

    // 注册，更新事件
    void enableRead();
    void disableRead();
    void enableWrite();
    void disableWrite();
    void enableClose();
    void disableClose();
    void enableError();
    void disableError();
    void disableAll();

    // 处理事件
    void handleEvent();

   private:
    EventLoop* loop_;                // 所属的EventLoop
    const int fd_;                   // 封装的套接字
    uint32_t events_ = kNoneEvent;   // 关心的事件
    uint32_t revents_ = kNoneEvent;  // 实际发生的事件
    int index_ = kNew;               // Channel在Poller中的索引

    // 回调函数
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    // 更新Channel
    void update();
};
}  // namespace reactor