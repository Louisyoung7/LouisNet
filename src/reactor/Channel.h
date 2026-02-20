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
    static constexpr int kErrorEvent = EPOLLERR;
    static constexpr int kCloseEvent = EPOLLHUP | EPOLLRDHUP;

    //* Channel状态枚举，用于Poller管理
    //* 这些状态是Channel在Poller中的生命周期状态
    static constexpr int kNew = -1;     ///< 未添加到Poller，也未注册到epoll
    static constexpr int kAdded = 0;    ///< 添加到Poller，也注册到epoll
    static constexpr int kDeleted = 1;  ///< 添加到Poller，但未注册到epoll

    // 构造析构
    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 禁用拷贝
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    // 获取套接字
    int fd() const {
        return fd_;
    }
    // 获取关心的事件
    uint32_t events() const {
        return events_;
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
    // 获取所属的EventLoop
    EventLoop* ownerLoop() const {
        return loop_;
    }

    // 检查是否没有事件
    bool isNoneEvent() const {
        return events_ == kNoneEvent;
    }
    // 检查是否关注了读或写事件
    bool isReading() const {
        return events_ & kReadEvent;
    }
    bool isWriting() const {
        return events_ & kWriteEvent;
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
    void enableRead() {
        events_ |= kReadEvent;
        update();
    }
    void disableRead() {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWrite() {
        events_ |= kWriteEvent;
        update();
    }
    void disableWrite() {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll() {
        events_ = kNoneEvent;
        update();
    }

    // 处理事件
    void handleEvent();

    // 移除Channel
    void remove();

   private:
    // 更新Channel
    void update();

    EventLoop* loop_;   // 所属的EventLoop
    const int fd_;      // 封装的套接字
    uint32_t events_;   // 关心的事件
    uint32_t revents_;  // 实际发生的事件
    int index_;         // Channel在Poller中的索引

    // 回调函数
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
}  // namespace reactor