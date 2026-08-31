#include "Channel.h"

#include <cassert>

#include "EventLoop.h"
#include "Poller.h"

using namespace net::reactor;

// 构造析构
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(kNoneEvent),
      revents_(kNoneEvent),
      index_(kNew),
      tied_(false),
      isInLoop_(false) {}
Channel::~Channel() { assert(!isInLoop_); }

// 处理事件
void Channel::handleEvents() {
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            handler();
        }
    } else {
        handler();
    }
}

// 处理真实返回的事件
void Channel::handler() {
    // 处理关闭事件
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        }
    }

    // 处理错误事件
    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    // 处理读取事件
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) {
            readCallback_();
        }
    }

    // 处理写入事件
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

// 从EventLoop中移除Channel
void Channel::remove() {
    isInLoop_ = false;
    loop_->removeChannel(this);
}

// 更新EventLoop中Channel的事件关注
void Channel::update() {
    isInLoop_ = true;
    loop_->updateChannel(this);
}

// 绑定回调执行对象
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}
