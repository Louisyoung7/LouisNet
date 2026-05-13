#include "SocketsOps.h"

#include <sys/socket.h>

#include "log/Logger.h"
#include "net/InetAddress.h"

using namespace net;

// 创建非阻塞的socket文件描述符
int sockets::createNonblockingSocket() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        logging::critical("{}-{}-{} createNonblockingSocket() failed to create socket: {}\n\n", __FILE__, __func__,
                          __LINE__, strerror(errno));
    }
    return fd;
}

// 连接对端
int sockets::connect(int sockfd, const InetAddress& peerAddr) {
    int ret = ::connect(sockfd, peerAddr.getSockaddr(), sizeof(struct sockaddr_in));
    if (ret < 0) {
        logging::critical("{}-{}-{} connect() failed to connect to {}: {}\n\n", __FILE__, __func__, __LINE__,
                          peerAddr.toIpPort(), strerror(errno));
    }
    return ret;
}

// 获取本端地址类
struct sockaddr_in sockets::getLocalAddr(int sockfd) {
    struct sockaddr_in localAddr {};
    socklen_t len = sizeof(localAddr);
    if (::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localAddr), &len) < 0) {
        logging::critical("{}-{}-{} getLocalAddr() failed to get local address: {}\n\n", __FILE__, __func__, __LINE__,
                          strerror(errno));
    }
    return localAddr;
}

// 获取对端地址类
struct sockaddr_in sockets::getPeerAddr(int sockfd) {
    struct sockaddr_in peerAddr {};
    socklen_t len = sizeof(peerAddr);
    if (::getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&peerAddr), &len) < 0) {
        logging::critical("{}-{}-{} getPeerAddr() failed to get peer address: {}\n\n", __FILE__, __func__, __LINE__,
                          strerror(errno));
    }
    return peerAddr;
}
