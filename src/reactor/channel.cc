#include "reactor/channel.h"

#include <cstdint>

#include "reactor/eventLoop.h"
#include "reactor/poller.h"

namespace reactor {
// 构造析构
Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {
}
Channel::~Channel() = default;

int Channel::fd() const {
    return fd_;
}

void Channel::setRevents(uint32_t revents) {
    revents_ = revents;
}

void Channel::setReadCallback(const EventCallback& callback) {
    readCallback_ = callback;
}

void Channel::setWriteCallback(const EventCallback& callback) {
    writeCallback_ = callback;
}

void Channel::enableRead() {
    events_ |= EPOLLIN;
    update();
}
void Channel::disableRead() {
    events_ &= ~EPOLLIN;
    update();
}
void Channel::enableWrite() {
    events_ |= EPOLLOUT;
    update();
}
void Channel::disableWrite() {
    events_ &= ~EPOLLOUT;
    update();
}
void Channel::disableAll() {
    events_ = kNoneEvent_;
    update();
}

void Channel::handleEvent() {
    if (revents_ & EPOLLIN) {
        if (readCallback_) {
            readCallback_();
        }
    } else if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

void Channel::update() {
    loop_->updateChannel(this);
}
}  // namespace reactor