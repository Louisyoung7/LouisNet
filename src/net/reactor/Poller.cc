#include "Poller.h"

#include <sys/epoll.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Channel.h"
#include "EventLoop.h"
#include "log/Logger.h"

using namespace net::reactor;

namespace {
std::string operationToString(int operation) {
    switch (operation) {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_MOD:
            return "MOD";
        case EPOLL_CTL_DEL:
            return "DEL";
        default:
            return "unknown operation";
    }
}
}  // namespace

// 构造析构
Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop), epollFd_(::epoll_create1(EPOLL_CLOEXEC)), events_(16) {
    if (epollFd_ == -1) {
        critical(
            "[Poller] Poller() failed to create epollfd: {}.\n\n",
            strerror(errno)
        );
        // 无法创建epollfd，程序无法运行
        abort();
    }
}
Poller::~Poller() { ::close(epollFd_); }

void Poller::poll(int timeoutMs, ChannelList& activeChannels) {
    // 获取活跃Channel的个数
    int nfds =
        ::epoll_wait(epollFd_, events_.data(), events_.size(), timeoutMs);

    //*  保存错误码，避免不小心调用系统调用，errno被覆盖
    int savedErrno = errno;

    if (nfds > 0) {
        // 填充活跃Channel列表
        fillActiveChannels(nfds, activeChannels);

        //* 如果活跃事件数量接近events_的大小，动态扩展events_向量
        if (static_cast<size_t>(nfds) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (nfds == 0) {
        debug("[Poller] poll() timed out.\n\n");
    } else {
        errno = savedErrno;
        error("[Poller] poll() failed: {}.\n\n", strerror(errno));
    }
}

void Poller::updateChannel(Channel* channel) {
    // 获取fd
    int fd = channel->fd();
    //* 这里没有保存events临时变量，调用channel->events()获取最新的关心事件
    if (channel->index() == Channel::kNew ||
        channel->index() == Channel::kDeleted) {
        if (channel->index() == Channel::kNew) {
            assert(fdMap_.find(fd) == fdMap_.end());
            fdMap_[fd] = channel;
        } else {
            assert(fdMap_.find(fd) != fdMap_.end());
            assert(fdMap_[fd] == channel);
        }

        update(EPOLL_CTL_ADD, channel);
        channel->setIndex(Channel::kAdded);
    } else {
        assert(fdMap_.find(fd) != fdMap_.end());
        assert(fdMap_[fd] == channel);
        assert(channel->index() == Channel::kAdded);

        //* 没有关注事件，移除监听，减少资源消耗
        if (channel->events() == Channel::kNoneEvent) {
            //* 只在epoll中移除，在map中仍存在
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(Channel::kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Poller::removeChannel(Channel* channel) {
    // 获取fd
    int fd = channel->fd();
    assert(fdMap_.find(fd) != fdMap_.end());
    assert(fdMap_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == Channel::kDeleted || index == Channel::kAdded);

    // 从map中删除
    fdMap_.erase(fd);

    // 从epoll中取消注册fd
    if (index == Channel::kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }

    channel->setIndex(Channel::kNew);
}

void Poller::fillActiveChannels(int nfds, ChannelList& activeChannels) const {
    // 遍历epoll_wait返回的事件，填充EventLoop的活跃Channel列表
    for (int i = 0; i < nfds; ++i) {
        // 获取channel
        auto channel = static_cast<Channel*>(events_[i].data.ptr);
        // 设置revents
        channel->setRevents(events_[i].events);
        // 将Channel添加到活跃Channel列表中
        activeChannels.push_back(channel);
    }
}

void Poller::update(int operation, Channel* channel) {
    struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;
    if (::epoll_ctl(epollFd_, operation, channel->fd(), &event) < 0) {
        error(
            "[Poller] update() failed to {} fd {} with errno: "
            "{}.\n\n",
            operationToString(operation), channel->fd(), strerror(errno)
        );
    }
}