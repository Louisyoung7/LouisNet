#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>   // for errno
#include <cstring>  // for strerror
#include <map>
#include <vector>

#include "base/noncopyable.h"

namespace net::reactor {
class Channel;
class EventLoop;

class Poller : public base::noncopyable {
    using ChannelList = std::vector<Channel*>;
    using ChannelMap = std::map<int, Channel*>;

   public:
    // 构造析构
    explicit Poller(EventLoop* loop);
    ~Poller();

    // 调用epoll_wait和fillActiveChannels，填充活跃Channel列表activeChannels
    void poll(int timeoutMs, ChannelList& activeChannels);

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

   private:
    // 填充EventLoop的活跃Channel列表
    void fillActiveChannels(int nfds, ChannelList& activeChannels) const;

    // 更新被监听fd的事件类型
    void update(int operation, Channel* channel);

    EventLoop* ownerLoop_;                    ///< 所在的EventLoop
    const int epollFd_;                       ///< epoll实例的文件描述符
    ChannelMap fdMap_;                        ///< 存储所有fd:Channel的映射
    std::vector<struct epoll_event> events_;  ///< 存储epoll_wait返回的事件
};
}  // namespace net::reactor
