#include "Connector.h"

#include <cassert>

#include "log/Logger.h"
#include "net/SocketsOps.h"
#include "net/reactor/Channel.h"
#include "net/reactor/EventLoop.h"

using namespace net;

Connector::Connector(reactor::EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop), serverAddr_(serverAddr), connected_(false), state_(State::kDisconnected) {
    debug("[Connector] Connector() serverAddr: {}\n\n", serverAddr_.toIpPort());
}

// 启动连接
void Connector::start() {
    connected_ = true;
    // runInLoop 强制在Loop线程执行
    // 如果当前线程不是Loop线程，会进入Loop线程的任务队列等待执行
    // 如果当前线程是Loop线程，会立即执行
    loop_->runInLoop([this]() { startInLoop(); });
}

// 停止连接
void Connector::stop() {
    connected_ = false;
    // queueInLoop 强制在Loop线程的任务队列延迟执行
    loop_->queueInLoop([this]() { stopInLoop(); });
}

// 重启连接，必须在事件循环中调用
void Connector::restart() {
    assert(loop_->isInLoopThread());
    // 重置状态
    state_ = State::kDisconnected;
    connected_ = true;
    startInLoop();
}

// Loop线程启动连接
void Connector::startInLoop() {
    assert(loop_->isInLoopThread());
    assert(state_ == State::kDisconnected);
    if (connected_) { connect(); }
}

// Loop线程停止连接
void Connector::stopInLoop() {
    assert(loop_->isInLoopThread());
    if (state_ == State::kConnecting) {
        state_ = State::kDisconnected;
        int sockfd = getSocket();
        assert(sockfd >= 0);
        ::close(sockfd);
    }
}

// 创建socket并尝试连接对端
void Connector::connect() {
    // 创建非阻塞的socket
    int sockfd = sockets::createNonblockingSocket();
    // 连接对端
    int ret = sockets::connect(sockfd, serverAddr_);
    int savedErrno = (ret == 0) ? 0 : errno;
    if (savedErrno == 0 || savedErrno == EINPROGRESS) {
        connecting(sockfd);
    } else {
        error("[Connector] connect() failed to connect to {}: {}\n\n", serverAddr_.toIpPort(), strerror(savedErrno));
        ::close(sockfd);
    }
}

// 连接过程中的操作
void Connector::connecting(int sockfd) {
    state_ = State::kConnecting;
    // 创建Channel，用于监听发起连接事件
    channel_.reset(new reactor::Channel(loop_, sockfd));
    // 关联Channel和Connector，确保在Channel事件发生时能够调用Connector的回调函数
    channel_->tie(shared_from_this());
    channel_->setWriteCallback([this]() { handleWrite(); });
    channel_->setErrorCallback([this]() { handleError(); });
    // 启用写事件，关心EPOLLOUT
    channel_->enableWrite();
}

// 获取已连接的socket fd，并从EventLoop中移除Channel
int Connector::getSocket() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    debug("[Connector] getSocket() sockfd: {}\n\n", sockfd);
    // 不能在此处重置Channel，当前可能位于Channel的handleEvents中
    // 需要 queueInLoop 强制在Loop线程的任务队列延迟执行
    loop_->queueInLoop([this]() { resetChannel(); });
    return sockfd;
}

// 重置Channel
void Connector::resetChannel() {
    channel_.reset();
    debug("[Connector] resetChannel() sockfd reset.\n\n");
}

// 处理写事件
// 取出已连接的socket fd，调用新连接回调函数
void Connector::handleWrite() {
    if (state_ == State::kConnecting) {
        state_ = State::kConnected;
        int sockfd = getSocket();
        assert(sockfd >= 0);
        newConnectionCallback_(sockfd);
    } else {
        assert(state_ == State::kDisconnected);
    }
}
// 处理错误事件
// 取出已连接的socket fd，关闭Channel和socket
void Connector::handleError() {
    assert(state_ == State::kConnecting);
    int sockfd = getSocket();
    assert(sockfd >= 0);
    ::close(sockfd);
}
