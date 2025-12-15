#pragma once

#include <sys/epoll.h>  // for epoll API
#include <unistd.h>     // for close

#include <cerrno>   // for errno
#include <cstring>  // for strerror
#include <iostream>
#include <stdexcept>  // for runtime_error
#include <string>
#include <vector>

namespace net {

/**
 * @brief Epoll类
 * 简单封装了epoll的常用API，包括添加，修改，删除，等待等等
 */
class Epoll {
   public:
    /**
     * @brief 构造函数，创建一个epoll实例
     * @param 指定epoll_wait最多能返回的事件数量
     * @throw runtime_error 创建失败
     */
    explicit Epoll(int maxEvents = 1024) : epfd_(-1) {
        epfd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epfd_ == -1) {
            //? runtime_error是什么？strerror是什么？
            throw std::runtime_error(std::string("create epoll instance failed: ") + ::strerror(errno));
        }

        // 初始化events向量，让向量中的所有epoll_event自动值初始化
        events_.resize(maxEvents);
    }

    /**
     * @brief 析构函数
     * 关闭epoll实例的文件描述符
     */
    ~Epoll() {
        if (epfd_ != -1) {
            ::close(epfd_);
        }
    }

    /**
     * @brief 添加fd
     * 添加要监听的fd，指定监听的事件，可以指定关联的数据指针
     * @param fd 要添加的文件描述符
     * @param events 指定的事件
     * @param data_ptr 数据指针，可以不指定
     * @return true 添加成功
     * @return false 添加失败，打印日志
     */
    bool addFd(int fd, uint32_t events, void* data_ptr = nullptr) {
        struct epoll_event ev {};
        ev.events = events;
        // 没有传数据指针，就用fd
        if (data_ptr == nullptr) {
            ev.data.fd = fd;
        }
        // 传了数据指针
        else {
            ev.data.ptr = data_ptr;
        }

        if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            std::cerr << "epoll_ctl add failed for fd=" << fd << ": " << ::strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    /**
     * @brief 修改fd
     * 修改指定fd对应的信息，包括监听的事件，附带的数据指针
     * @param fd 要修改的文件描述符
     * @param events 新的事件
     * @param data_ptr 新的数据指针，可以不指定
     * @return true 修改成功
     * @return false 修改失败，打印日志
     */
    bool modFd(int fd, uint32_t events, void* data_ptr = nullptr) {
        struct epoll_event ev {};
        ev.events = events;  // 设置新事件
        if (data_ptr) {
            ev.data.ptr = data_ptr;  // 设置新的数据指针
        } else {
            ev.data.fd = fd;  // 避免fd被意外设置为0
        }

        if (::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
            std::cerr << "epoll_ctl mod failed for fd=" << fd << ": " << ::strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    /**
     * @brief 删除fd
     * 删除指定fd
     * @param fd 要删除的文件描述符
     * @return true 删除成功
     * @return false 删除失败，打印日志
     */
    bool delFd(int fd) {
        if (::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
            std::cerr << "epoll_ctl del failed for fd=" << fd << ": " << ::strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    /**
     * @brief 等待epoll返回就绪的事件列表
     * @param timeoutMs 阻塞的时间（毫秒），0为直接返回不等待，-1表示一直等待
     * @return -1 等待出错，打印日志
     * @return numEvents 就绪的事件数量
     */
    int wait(int timeoutMs = -1) {
        // 这里传&events_[0]是可以的，因为vector底层的数组的内存是连续的，而前面通过resize初始化了，所以这里不会发生越界
        int numEvents = epoll_wait(epfd_, &events_[0], events_.size(), timeoutMs);

        if (numEvents == -1) {
            // 忽略信号中断的错误
            if (errno != EINTR) {
                std::cerr << "epoll_wait failed:" << ::strerror(errno) << std::endl;
            }
            return -1;  // 表示出错
        }

        // 成功返回就绪事件的数量
        return numEvents;
    }

    /**
     * @brief 获取就绪的事件向量
     * @return events_ 就绪的事件向量
     */
    const std::vector<struct epoll_event>& getEvents() const {
        return events_;
    }

   private:
    int epfd_;                           ///< epoll实例的文件描述符
    std::vector<struct epoll_event> events_;  ///< 存储epoll_wait返回的事件的缓冲区
};
}  // namespace net
