#include "TcpServer.h"

#include <unistd.h>

#include <cassert>
#include <memory>

#include "log/Logger.h"
#include "net/Acceptor.h"
#include "net/TcpConnection.h"
#include "net/reactor/EventLoop.h"
#include "net/reactor/EventLoopThreadPool.h"

using namespace net;
using namespace net::reactor;
using namespace base;

// 初始化TcpServer，并设置Acceptor实例的新连接回调函数
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, bool reusePort)
    : mainLoop_(loop),
      listenAddr_(listenAddr),
      name_(name),
      acceptor_(std::make_unique<Acceptor>(loop, listenAddr, reusePort)),
      threadPool_(std::make_unique<EventLoopThreadPool>(loop)) {
    // 新连接回调函数需要Acceptor获取的通信套接字和对端地址作为参数
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress& peerAddr) { onNewConnection(sockfd, peerAddr); });
}

// 将仍存储在TcpServer的所有TcpConnection连接实例销毁
TcpServer::~TcpServer() {
    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        // 在IO线程销毁连接
        conn->getLoop()->runInLoop([conn]() { conn->connectionDestroyed(); });
        conn.reset();
    }
}

// 启动服务器
// 让Acceptor实例开始监听新连接
void TcpServer::start() {
    debug("[TcpServer] start() starting to listen on {}.\n\n", listenAddr_.toIpPort());
    threadPool_->start();
    mainLoop_->runInLoop([this]() { acceptor_->listen(); });
}

// 设置IO线程池的线程数量
void TcpServer::setThreadNum(int numThreads) { threadPool_->setThreadNum(numThreads); }

// 处理新连接
// 会被设置为Acceptor实例的新连接回调函数，在后续有新连接时被调用
void TcpServer::onNewConnection(int sockfd, const InetAddress& peerAddr) {
    try {
        // 从线程池中获取一个IO线程
        EventLoop* ioLoop = threadPool_->getNextLoop();
        // 创建新TcpConnection实例
        TcpConnectionPtr conn = std::make_shared<TcpConnection>(ioLoop, sockfd, listenAddr_, peerAddr);
        // 设置连接回调
        conn->setConnectionCallback([this](const TcpConnectionPtr& conn) { onConnection(conn); });
        // 设置消息接收回调
        conn->setMessageCallback(
            [this](const TcpConnectionPtr& conn, Buffer& buffer) { messageCallback_(conn, buffer); });
        // 设置写完成回调
        conn->setWriteCompleteCallback([this](const TcpConnectionPtr& conn) {
            if (writeCompleteCallback_) {
                writeCompleteCallback_(conn);
            }
        });
        // 设置关闭回调
        conn->setCloseCallback([this](const TcpConnectionPtr& conn) { removeConnection(conn); });

        // 在IO线程建立连接
        ioLoop->runInLoop([conn]() { conn->connectionEstablished(); });

        // 存储实例
        connections_[sockfd] = conn;
    } catch (const std::exception& e) {
        error("[TcpServer] onNewConnection() error: {}.\n\n", e.what());
        // 发生错误时关闭socket
        ::close(sockfd);
    }
}
// 处理连接回调
// 在onNewConnection中被设置为新创建的TcpConnection的回调函数
void TcpServer::onConnection(const TcpConnectionPtr& conn) {
    try {
        // 调用上层设置的回调
        if (connectionCallback_) {
            connectionCallback_(conn);
        }
    } catch (const std::exception& e) {
        error("[TcpServer] onConnection() error: {}.\n\n", e.what());
    }
}

// 移除连接
void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    mainLoop_->runInLoop([this, conn]() { removeConnectionInLoop(conn); });
}
// 在IO线程移除连接
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    assert(mainLoop_->isInLoopThread());
    // 从map中移除
    connections_.erase(conn->fd());
    // 获取连接所属的IO线程
    EventLoop* ioLoop = conn->getLoop();
    // 在IO线程销毁连接
    ioLoop->queueInLoop([conn]() { conn->connectionDestroyed(); });
}