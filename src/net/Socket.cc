#include "Socket.h"

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "InetAddress.h"
#include "log/Logger.h"

namespace net {
// 创建非阻塞的socket文件描述符
int createNonblockingSocket() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        logging::critical("{}-{}-{} createNonblockingSocket() failed to create socket: {}\n\n", __FILE__, __func__,
                          __LINE__, strerror(errno));
    }
    return fd;
}

// 连接对端
int connectPeer(int sockfd, const InetAddress& peerAddr) {
    return ::connect(sockfd, peerAddr.getSockaddr(), sizeof(struct sockaddr_in));
}

Socket::~Socket() { ::close(sockfd_); }

// 绑定IP and Port
void Socket::bindAddress(const InetAddress& localAddr) {
    if (::bind(sockfd_, localAddr.getSockaddr(), sizeof(struct sockaddr_in)) < 0) {
        logging::critical("[Socket] bindAddress() failed, sockfd: {}.\n\n", sockfd_);
    }
}

// 设置监听
void Socket::listen() {
    if (::listen(sockfd_, SOMAXCONN) < 0) { logging::critical("[Socket] listen() failed, sockfd: {}.\n\n", sockfd_); }
}

// 接受连接
int Socket::accept(InetAddress& peerAddr) {
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int connfd = ::accept4(sockfd_, const_cast<struct sockaddr*>(peerAddr.getSockaddr()), &addrLen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) { logging::error("[Socket] accept() failed, sockfd: {}.\n\n", sockfd_); }
    return connfd;
}

// 关闭写端
void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        logging::error("[Socket] shutdownWrite() failed, sockfd: {}.\n\n", sockfd_);
    }
}

// 设置Nagle算法
void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
// 设置复用地址
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
// 设置复用端口
void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
// 设置保持连接
void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

// 获取错误码
int Socket::getError() const {
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) { return errno; }
    return optval;
}
}  // namespace net