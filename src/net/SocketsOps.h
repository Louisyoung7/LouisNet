#pragma once

#include <arpa/inet.h>

namespace net {
class InetAddress;

namespace sockets {
// 创建非阻塞的socket文件描述符
int createNonblockingSocket();

// 连接对端
int connect(int sockfd, const InetAddress& peerAddr);

// 获取本端地址类
struct sockaddr_in getLocalAddr(int sockfd);

// 获取对端地址类
struct sockaddr_in getPeerAddr(int sockfd);
}  // namespace sockets
}  // namespace net