#include "Acceptor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "InetAddress.h"
#include "base/LouisLog.h"
#include "reactor/Channel.h"
#include "reactor/EventLoop.h"

using namespace net;
using namespace net::reactor;

// 创建非阻塞的socket文件描述符
static int createNonblockingSocket() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        FATAL_F("%s-%s-%d createNonblockingSocket() failed to create socket: %s\n\n", __FILE__, __func__, __LINE__,
                strerror(errno));
    }
    return fd;
}
// 构造析构
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort)
    : loop_(loop),
      listening_(false),
      acceptSocket_(std::make_unique<Socket>(createNonblockingSocket())),
      acceptChannel_(std::make_unique<Channel>(loop, acceptSocket_->fd())) {
    // 绑定IP and Port
    acceptSocket_->bindAddress(listenAddr);
    // 设置复用地址和端口，设置Nagle算法
    acceptSocket_->setReuseAddr(true);
    acceptSocket_->setReusePort(reusePort);
    acceptSocket_->setTcpNoDelay(true);

    // 设置Channel的读回调，处理新连接
    acceptChannel_->setReadCallback([this]() { handleRead(); });

    DEBUG_F("[Acceptor] Acceptor() created with listenFd_ = %d\n\n", acceptSocket_->fd());
}
Acceptor::~Acceptor() {
    DEBUG_F("[Acceptor] ~Acceptor() closing listenFd_ = %d\n\n", acceptSocket_->fd());
    // 关闭监听Channel
    acceptChannel_->disableAll();
    acceptChannel_->remove();
}

// 开始监听
void Acceptor::listen() {
    assert(!listening_);
    acceptSocket_->listen();
    acceptChannel_->enableRead();
    listening_ = true;
    DEBUG_F("[Acceptor] listen() listenFd_ = %d\n\n", acceptSocket_->fd());
}

// 处理监听的socket上的读事件（新连接）
void Acceptor::handleRead() {
    // 接受新连接
    InetAddress peerAddr;
    int connfd = acceptSocket_->accept(peerAddr);
    if (connfd > 0) {
        DEBUG_F("[Acceptor] handleRead() accepted connection with fd = %d, from %s\n\n", connfd,
                peerAddr.toIpPort().c_str());

        // 调用新连接回调函数
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            // 没有新连接回调函数，关闭连接
            ::close(connfd);
        }
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            DEBUG_F("[Acceptor] handleRead() no more connections\n\n");
        } else {
            ERROR_F("[Acceptor] handleRead() accept() failed with errno = %s\n\n", strerror(errno));
        }
    }
}