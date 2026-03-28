#pragma once

namespace net {
class InetAddress;

// 封装套接字类
// 1.利用RAII自动管理资源
// 2.类型安全，避免误用（类型转换和拷贝操作）
// 3.提供语义明确的API
// 4.封装复杂的accept4系统调用

class Socket {
    const int sockfd_;

   public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {
    }
    ~Socket();

    // 返回套接字
    int fd() const {
        return sockfd_;
    }

    // 绑定IP and Port
    void bindAddress(const InetAddress& localAddr);

    // 设置监听
    void listen();

    // 接受连接
    int accept(InetAddress& peerAddr);

    // 关闭写端
    void shutdownWrite();

    // 设置Nagle算法
    void setTcpNoDelay(bool on);
    // 设置复用地址
    void setReuseAddr(bool on);
    // 设置复用端口
    void setReusePort(bool on);
    // 设置保持连接
    void setKeepAlive(bool on);

   private:
};
}  // namespace net