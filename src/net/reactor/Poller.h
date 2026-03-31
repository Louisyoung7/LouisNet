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

// Poller类
// 1.封装epoll_wait等epoll相关调用
// 2.管理所有注册在epoll中的fd:Channel的映射
// 3.等待事件响应，填充活跃Channel列表active_channels，包含所有实际发生的事件
// 4.提供Channel的添加、修改、删除接口

class Poller : public base::noncopyable {
    using ChannelList = std::vector<Channel*>;
    using ChannelMap = std::map<int, Channel*>;

    EventLoop* owner_loop_;                   ///< 所在的EventLoop
    const int epollFd_;                       ///< epoll实例的文件描述符
    ChannelMap fd_channel_map_;               ///< 存储所有fd:Channel的映射
    std::vector<struct epoll_event> events_;  ///< 存储epoll_wait返回的事件

   public:
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
    void addFd(int fd, int events);
    void modFd(int fd, int events);
    void delFd(int fd);

    // 填充EventLoop的活跃Channel列表
    void fillActiveChannels(int nfds, ChannelList& active_channels) const;
};
}  // namespace net::reactor
