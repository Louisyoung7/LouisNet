#include "Acceptor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "InetAddress.h"
#include "reactor/Channel.h"
#include "reactor/EventLoop.h"
using std::cerr;
using std::cout;
using std::endl;

namespace net {
// 构造析构
Acceptor::Acceptor(reactor::EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      listenAddr_(listenAddr),
      listening_(false),
      listenFd_(createNonblockingSocket()),
      acceptChannel_(std::make_unique<reactor::Channel>(loop, listenFd_)) {
    // 设置Channel的读回调，处理新连接
    acceptChannel_->setReadCallback([this]() { handleRead(); });
    cout << "[Acceptor] Acceptor() created with listenFd_ = " << listenFd_ << endl << endl;
}
Acceptor::~Acceptor() {
    // 关闭监听Channel
    acceptChannel_->disableAll();
    acceptChannel_->remove();
    ::close(listenFd_);
    cerr << "[Acceptor] ~Acceptor() closing listenFd_ = " << listenFd_ << endl << endl;
}

// 开始监听
void Acceptor::listen() {
    assert(!listening_);
    if (::bind(listenFd_, listenAddr_.getSockaddr(), sizeof(struct sockaddr_in)) < 0) {
        cerr << "[Acceptor] listen() bind() failed" << endl << endl;
    }
    if (::listen(listenFd_, SOMAXCONN) < 0) {
        cerr << "[Acceptor] listen() bind() failed" << endl << endl;
    } else {
        listening_ = true;
    }

    // 开启监听Channel的读事件
    acceptChannel_->enableRead();

    cout << "[Acceptor] listen() listening on " << listenAddr_.toIpPort() << endl << endl;
}

// 创建非阻塞的socket文件描述符
int Acceptor::createNonblockingSocket() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        cerr << "[Acceptor] createNonblockingSocket() failed to create socket: " << strerror(errno) << endl << endl;
        // 无法创建socket，程序无法继续运行
        abort();
    }

    // 设置socket选项，允许地址复用
    int optval = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        cerr << "[Acceptor] createNonblockingSocket() failed to set SO_REUSEADDR: " << strerror(errno) << endl << endl;
        // 这个错误不是致命的，继续创建socket
    }
    return fd;
}

// 接受新连接
int Acceptor::accept(InetAddress& peerAddr) {
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int connfd = ::accept4(listenFd_, const_cast<struct sockaddr*>(peerAddr.getSockaddr()), &addrLen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        cerr << "[Acceptor] accept() failed" << endl << endl;
    }
    return connfd;
}

// 处理监听的socket上的读事件（新连接）
void Acceptor::handleRead() {
    // 接受新连接，现在是单线程环境，单次接受避免阻塞
    InetAddress peerAddr;
    int connfd = accept(peerAddr);
    if (connfd > 0) {
        cout << "[Acceptor] handleRead() accepted connection with fd = " << connfd << " from " << peerAddr.toIpPort()
             << endl
             << endl;

        // 调用新连接回调函数
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            // 没有新连接回调函数，关闭连接
            ::close(connfd);
        }
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            cout << "[Acceptor] handleRead() no more connections" << endl << endl;
        } else {
            cerr << "[Acceptor] handleRead() accept() failed with errno = " << strerror(errno) << endl << endl;
        }
    }
}
}  // namespace net