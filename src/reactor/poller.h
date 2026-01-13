#pragma once

#include <sys/epoll.h>  // for epoll API
#include <unistd.h>     // for close

#include <cerrno>   // for errno
#include <cstring>  // for strerror
#include <map>
#include <vector>

namespace reactor {
class Channel;
class EventLoop;

class Poller {
   public:
    // 构造析构
    explicit Poller(EventLoop* loop);
    ~Poller();

    // 调用epoll_wait，等待事件响应
    void poll(int timeout_ms, std::vector<Channel*>& active_channels);

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

   private:
    EventLoop* owner_loop_;                    ///< 所在的EventLoop
    int epollFd_;                              ///< epoll实例的文件描述符
    std::map<int, Channel*> fd_channel_map_;   ///< 存储所有fd:Channel的映射
    std::vector<struct epoll_event> revents_;  ///< 存储epoll_wait返回的真实发生的事件

   private:
    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);
};
}  // namespace reactor
