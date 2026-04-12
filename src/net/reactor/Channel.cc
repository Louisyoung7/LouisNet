#include "Channel.h"

#include <cassert>

#include "EventLoop.h"
#include "Poller.h"

using namespace net::reactor;

// 构造析构
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(kNoneEvent), revents_(kNoneEvent), index_(kNew) {
}
Channel::~Channel() {
    assert(index_ == kNew);
    // 取消关注所有事件
    disableAll();
    remove();
}

void Channel::handleEvent() {
    // 执行实际发生的事件回调
    if (revents_ & kReadEvent) {
        if (readCallback_) {
            readCallback_();
        }
    }
    if (revents_ & kWriteEvent) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
    if (revents_ & kCloseEvent) {
        if (closeCallback_) {
            closeCallback_();
        }
    }
    if (revents_ & kErrorEvent) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
}

// 从EventLoop中移除Channel
void Channel::remove() {
    loop_->removeChannel(this);
}

// 更新EventLoop中Channel的事件关注
void Channel::update() {
    loop_->updateChannel(this);
}