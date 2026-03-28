#include "Socket.h"

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "InetAddress.h"
#include "base/LouisLog.h"

namespace net {
Socket::~Socket() {
    ::close(sockfd_);
}

// 绑定IP and Port
void Socket::bindAddress(const InetAddress& localAddr) {
    if (::bind(sockfd_, localAddr.getSockaddr(), sizeof(struct sockaddr_in)) < 0) {
        FATAL_F("[Socket] bindAddress() failed, sockfd: %d.\n\n", sockfd_);
    }
}

// 设置监听
void Socket::listen() {
    if (::listen(sockfd_, SOMAXCONN) < 0) {
        FATAL_F("[Socket] listen() failed, sockfd: %d.\n\n", sockfd_);
    }
}

// 接受连接
int Socket::accept(InetAddress& peerAddr) {
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int connfd = ::accept4(sockfd_, const_cast<struct sockaddr*>(peerAddr.getSockaddr()), &addrLen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        ERROR_F("[Socket] accept() failed, sockfd: %d.\n\n", sockfd_);
    }
    return connfd;
}

// 关闭写端
void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        ERROR_F("[Socket] shutdownWrite() failed, sockfd: %d.\n\n", sockfd_);
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
}  // namespace net