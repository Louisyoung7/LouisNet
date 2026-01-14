#pragma once

class Channel;

namespace net {
class Socket {
   public:
    // 构造析构
    explicit Socket(int sockFd);
    ~Socket();

    // getters
    int fd() const {
        return sockFd_;
    }

    // 绑定地址
    void bindAddress(const Channel& addr);

    // 监听
    void listen(int backlog);

    // 接受连接
    int accept(Channel& addr);

    // 关闭写方向
    void shutdownWrite();

    // 设置 socket 选项
    // 设置 TCP_NODELAY 选项
    bool setTcpNoDelay(bool on);
    // 设置 SO_REUSEADDR 选项
    bool setReuseAddr(bool on);
    // 设置 SO_REUSEPORT 选项
    bool setReusePort(bool on);
    // 设置 SO_KEEPALIVE 选项
    bool setKeepAlive(bool on);

   private:
    const int sockFd_;
};
}  // namespace net