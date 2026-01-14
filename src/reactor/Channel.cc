#include "Channel.h"

#include "EventLoop.h"
#include "Poller.h"

namespace reactor {
// 构造析构
Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {
}
Channel::~Channel() {
    if (index_ != kNew) {
        loop_->removeChannel(this);
    }
}

void Channel::enableRead() {
    events_ |= kReadEvent;
    update();
}
void Channel::disableRead() {
    events_ &= ~kReadEvent;
    update();
}
void Channel::enableWrite() {
    events_ |= kWriteEvent;
    update();
}
void Channel::disableWrite() {
    events_ &= ~kWriteEvent;
    update();
}
void Channel::enableClose() {
    events_ |= kCloseEvent;
    update();
}
void Channel::disableClose() {
    events_ &= ~kCloseEvent;
    update();
}
void Channel::enableError() {
    events_ |= kErrorEvent;
    update();
}
void Channel::disableError() {
    events_ &= ~kErrorEvent;
    update();
}
void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

void Channel::handleEvent() {
    if (revents_ & kReadEvent) {
        if (readCallback_) {
            readCallback_();
        }
    } else if (revents_ & kWriteEvent) {
        if (writeCallback_) {
            writeCallback_();
        }
    } else if (revents_ & kCloseEvent) {
        if (closeCallback_) {
            closeCallback_();
        }
    } else if (revents_ & kErrorEvent) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
}

void Channel::update() {
    loop_->updateChannel(this);
}
}  // namespace reactor