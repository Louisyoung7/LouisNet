#include "Poller.h"

#include <sys/epoll.h>

#include <cstdint>
#include <iostream>

#include "Channel.h"
#include "EventLoop.h"

using std::cerr;
using std::cout;
using std::endl;

namespace reactor {
// 构造析构
Poller::Poller(EventLoop* loop) : owner_loop_(loop), epollFd_(epoll_create1(EPOLL_CLOEXEC)) {
}
Poller::~Poller() {
    ::close(epollFd_);
}

// 调用epoll_wait，等待事件响应
void Poller::poll(int timeout_ms, std::vector<Channel*>& active_channels) {
    // 获取活跃Channel的个数
    int nfds = ::epoll_wait(epollFd_, &events_[0], events_.size(), timeout_ms);
    // 遍历epoll_wait返回的事件向量
    for (int i = 0; i < nfds; ++i) {
        // 获取fd和返回的事件
        int fd = events_[i].data.fd;
        int events = events_[i].events;
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
    cout << "[Poller] updateChannel() fd: " << channel->fd() << " events: " << channel->events() << endl << endl;

    // 获取fd
    int fd = channel->fd();

    // 如果是新状态，添加到epoll中
    if (channel->index() == Channel::kNew) {
        
        addFd(fd, channel->events());
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

void Poller::addFd(int fd, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        cerr << "[Poller] addFd() failed to add fd " << fd << " to epollfd " << epollFd_
             << " errno: " << strerror(errno) << endl
             << endl;
    } else {
        cout << "[Poller] addFd() success to add fd " << fd << " to epollfd " << epollFd_ << endl << endl;
    }
}

void Poller::modFd(int fd, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
        cerr << "[Poller] modFd() failed to mod fd " << fd << " to epollfd " << epollFd_
             << " errno: " << strerror(errno) << endl
             << endl;
    } else {
        cout << "[Poller] modFd() success to mod fd " << fd << " to epollfd " << epollFd_ << endl << endl;
    }
}

void Poller::delFd(int fd) {
    if (::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        cerr << "[Poller] delFd() failed to del fd " << fd << " from epollfd " << epollFd_
             << " errno: " << strerror(errno) << endl
             << endl;
    } else {
        cout << "[Poller] delFd() success to del fd " << fd << " from epollfd " << epollFd_ << endl << endl;
    }
}

// 填充EventLoop的活跃Channel列表
void Poller::fillActiveChannels(int nfds, ChannelList& active_channels) const {
    // 遍历epoll_wait返回的事件，填充EventLoop的活跃Channel列表
    for (int i = 0; i < nfds; ++i) {
        // 获取fd和event
        int fd = events_[i].data.fd;
        uint32_t events = events_[i].events;

        // 在fd_channel_map_中查找对应fd的Channel
        auto it = fd_channel_map_.find(fd);
        // 存在
        if (it != fd_channel_map_.end()) {
            // 保存Channel
            Channel* channel = it->second;
            // 设置revents
            channel->setRevents(events);
            // 将Channel添加到活跃Channel列表中
            active_channels.push_back(channel);
        }
    }
}
}  // namespace reactor