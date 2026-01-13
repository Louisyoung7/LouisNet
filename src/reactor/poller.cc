#include "reactor/poller.h"

#include <sys/epoll.h>

#include <cstdint>

#include "reactor/channel.h"

namespace reactor {
// 构造析构
Poller::Poller(EventLoop* loop) : owner_loop_(loop), epollFd_(epoll_create1(EPOLL_CLOEXEC)) {
    // 为revents预先分配大小
    revents_.resize(1024);
}
Poller::~Poller() {
    if (epollFd_ != -1) {
        ::close(epollFd_);
    }
}

// 调用epoll_wait，等待事件响应
void Poller::poll(int timeout_ms, std::vector<Channel*>& active_channels) {
    // 获取活跃Channel的个数
    int nfds = ::epoll_wait(epollFd_, &revents_[0], revents_.size(), timeout_ms);
    // 遍历epoll_wait返回的事件向量
    for (int i = 0; i < nfds; ++i) {
        // 获取fd和返回的事件
        int fd = revents_[i].data.fd;
        int events = revents_[i].events;
        // 根据fd获取Channel
        Channel* channel = fd_channel_map_[fd];
        // 设置真实发生的事件
        channel->setRevents(events);
        // 添加到活跃列表
        active_channels.emplace_back(std::move(channel));
    }
}

// 更新Channel
void Poller::updateChannel(Channel* channel) {
    // 获取Channel的fd和关心的事件
    int fd = channel->fd();
    uint32_t events = channel->events();
    // 先看在map中有没有
    auto it = fd_channel_map_.find(fd);
    if (it == fd_channel_map_.end()) {
        // 没找到就添加，有事件才添加
        if (events != Channel::kNoneEvent_) {
            // 添加映射
            fd_channel_map_[fd] = channel;
            // 添加到epoll中
            addFd(fd, events);
        }
    } else {
        // 找到了，在epoll中修改
        if (events != Channel::kNoneEvent_) {
            modFd(fd, events);
        } else {
            // 这里只是在epoll中取消注册，并没有从map中删除
            delFd(fd);
        }
    }
}

// 移除Channel
void Poller::removeChannel(Channel* channel) {
    // 获取fd
    int fd = channel->fd();
    // 在map中找
    auto it = fd_channel_map_.find(fd);
    if (it != fd_channel_map_.end()) {
        delFd(fd);
        fd_channel_map_.erase(it);
    }
}

bool Poller::addFd(int fd, uint32_t events) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) != -1;
}

bool Poller::modFd(int fd, uint32_t events) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) != -1;
}

bool Poller::delFd(int fd) {
    return epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
}
}  // namespace reactor