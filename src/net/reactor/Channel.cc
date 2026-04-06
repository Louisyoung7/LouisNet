#include "Channel.h"

#include "EventLoop.h"
#include "Poller.h"

using namespace net::reactor;

// 构造析构
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(kNoneEvent), revents_(kNoneEvent), index_(kNew) {
}
Channel::~Channel() {
    if (index_ != kNew) {
        // 确保Channel没有注册任何事件
        disableAll();
        remove();
    }
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

// 移除Channel
void Channel::remove() {
    loop_->removeChannel(this);
}

// 更新Channel
void Channel::update() {
    loop_->updateChannel(this);
}