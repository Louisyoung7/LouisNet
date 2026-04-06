#include "Poller.h"

#include <sys/epoll.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Channel.h"
#include "EventLoop.h"
#include "base/LouisLog.h"

using namespace net::reactor;

// 构造析构
Poller::Poller(EventLoop* loop) : owner_loop_(loop), epollFd_(::epoll_create1(EPOLL_CLOEXEC)), events_(16) {
    if (epollFd_ == -1) {
        FATAL_F("[Poller] Poller() failed to create epollfd: %s.\n\n", strerror(errno));
        // 无法创建epollfd，程序无法运行
        abort();
    }
}
Poller::~Poller() {
    // 确保所有channel都从Poller中移除
    for (const auto& pair : fd_channel_map_) {
        Channel* channel = pair.second;
        assert(channel->index() == Channel::kNew);
    }
    ::close(epollFd_);
}

// 调用epoll_wait，等待事件响应，填充活跃Channel列表active_channels
void Poller::poll(int timeout_ms, ChannelList& active_channels) {
    DEBUG_F("[Poller] poll() called, timeout_ms = %d.\n\n", timeout_ms);
    // 获取活跃Channel的个数
    int nfds = ::epoll_wait(epollFd_, events_.data(), events_.size(), timeout_ms);
    //*  保存错误码，避免不小心调用系统调用，errno被覆盖
    int savedErrno = errno;

    if (nfds > 0) {
        DEBUG_F("[Poller] poll() returned %d events.\n\n", nfds);
        // 填充活跃channel列表
        fillActiveChannels(nfds, active_channels);

        //* 如果活跃事件数量接近events_的大小，动态扩展events_向量
        if (static_cast<size_t>(nfds) == events_.size()) {
            events_.resize(events_.size() * 2);
            DEBUG_F("[Poller] poll() expanded events_ vector to size %ld.\n\n", events_.size());
        } else if (nfds == 0) {
            DEBUG_F("[Poller] poll() timed out.\n\n");
        }
        // 处理错误
        else {
            if (savedErrno != EINTR) {
                errno = savedErrno;
                ERROR_F("[Poller] poll() failed: %s.\n\n", strerror(errno));
            }
            //* 如果errno是EINTR，说明是信号导致的中断，不是错误
            else {
                DEBUG_F("[Poller] poll() interrupted by signal.\n\n");
            }
        }
    }
}

// 更新Channel
void Poller::updateChannel(Channel* channel) {
    DEBUG_F("[Poller] updateChannel() fd: %d events: %d.\n\n", channel->fd(), channel->events());

    // 获取fd
    int fd = channel->fd();
    //* 这里没有保存events临时变量，调用channel->events()获取最新的关心事件
    // 新的Channel，添加到epoll和fd_channel_map中
    if (channel->index() == Channel::kNew) {
        // 断言Channel在map中原本不存在
        assert(fd_channel_map_.find(fd) == fd_channel_map_.end());
        // 添加到map中
        fd_channel_map_[fd] = channel;
        // 添加到epoll中
        addFd(fd, channel->events());
        // 更新channel状态
        channel->setIndex(Channel::kAdded);
    }
    // 已经存在的Channel，更新事件
    else {
        // 断言channel已经在map中
        assert(fd_channel_map_.find(fd) != fd_channel_map_.end());
        assert(fd_channel_map_[fd] == channel);

        //* 没有事件，移除监听，减少资源消耗
        if (channel->events() == Channel::kNoneEvent) {
            //* 只在epoll中移除，在map中仍存在
            delFd(fd);
            // 更新channel状态
            channel->setIndex(Channel::kDeleted);
        }
        // 有事件，更新
        else {
            modFd(fd, channel->events());
            channel->setIndex(Channel::kAdded);
        }
    }
}

// 移除Channel
void Poller::removeChannel(Channel* channel) {
    DEBUG_F("[Poller] removeChannel() called, fd = %d.\n\n", channel->fd());
    // 获取fd
    int fd = channel->fd();
    // 断言channel在map中存在
    assert(fd_channel_map_.find(fd) != fd_channel_map_.end());
    assert(fd_channel_map_[fd] == channel);

    // 确保Channel没有注册任何事件
    if (!channel->isNoneEvent()) {
        // 禁用所有事件
        channel->disableAll();
    }

    assert(channel->isNoneEvent());
    // 从epoll中取消注册fd
    delFd(fd);
    // 从map中删除
    fd_channel_map_.erase(fd);
    // 更新channel状态
    channel->setIndex(Channel::kNew);
}

void Poller::addFd(int fd, int events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        ERROR_F("[Poller] addFd() failed to add fd %d to epollfd %d, errno: %s.\n\n", fd, epollFd_, strerror(errno));
    } else {
        DEBUG_F("[Poller] addFd() success to add fd %d to epollfd %d.\n\n", fd, epollFd_);
    }
}

void Poller::modFd(int fd, int events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
        ERROR_F("[Poller] modFd() failed to mod fd %d to epollfd %d, errno: %s.\n\n", fd, epollFd_, strerror(errno));
    } else {
        DEBUG_F("[Poller] modFd() success to mod fd %d to epollfd %d.\n\n", fd, epollFd_);
    }
}

void Poller::delFd(int fd) {
    if (::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        ERROR_F("[Poller] delFd() failed to del fd %d from epollfd %d, errno: %s.\n\n", fd, epollFd_, strerror(errno));
    } else {
        DEBUG_F("[Poller] delFd() success to del fd %d from epollfd %d.\n\n", fd, epollFd_);
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