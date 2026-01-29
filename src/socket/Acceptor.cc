#include "Acceptor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
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
}  // namespace net