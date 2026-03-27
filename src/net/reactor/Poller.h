#pragma once

#include <sys/epoll.h>  // for epoll API
#include <unistd.h>     // for close

#include <cerrno>   // for errno
#include <cstring>  // for strerror
#include <map>
#include <vector>

#include "base/noncopyable.h"

namespace net::reactor {
class Channel;
class EventLoop;

class Poller : public base::noncopyable {
   public:
    using ChannelList = std::vector<Channel*>;
    using ChannelMap = std::map<int, Channel*>;
    // 构造析构
    explicit Poller(EventLoop* loop);
    ~Poller();

    // 调用epoll_wait和fillActiveChannels，填充活跃Channel列表active_channels
    void poll(int timeout_ms, ChannelList& active_channels);

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

   private:
    // 封装epoll的添加、修改、删除事件
    void addFd(int fd, uint32_t events);
    void modFd(int fd, uint32_t events);
    void delFd(int fd);

    // 填充EventLoop的活跃Channel列表
    void fillActiveChannels(int nfds, ChannelList& active_channels) const;

    EventLoop* owner_loop_;                   ///< 所在的EventLoop
    const int epollFd_;                       ///< epoll实例的文件描述符
    ChannelMap fd_channel_map_;               ///< 存储所有fd:Channel的映射
    std::vector<struct epoll_event> events_;  ///< 存储epoll_wait返回的事件
};
}  // namespace net::reactor
