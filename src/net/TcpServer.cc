#include "TcpServer.h"

#include <unistd.h>

#include <cassert>
#include <memory>

#include "log/Logger.h"
#include "net/Acceptor.h"
#include "net/TcpConnection.h"

using namespace net;
using namespace net::reactor;
using namespace base;

// 构造函数
// 初始化TcpServer，并设置Acceptor实例的新连接回调函数
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
    : loop_(loop), listenAddr_(listenAddr), name_(name), acceptor_(std::make_unique<Acceptor>(loop, listenAddr, true)) {
    // 设置Acceptor实例的新连接回调函数
    // 新连接回调函数需要Acceptor获取的通信套接字和对端地址作为参数
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress& peerAddr) { onNewConnection(sockfd, peerAddr); });
}

// 析构函数
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
    loop_->runInLoop([this]() { acceptor_->listen(); });
}

// 处理新连接
// 会被设置为Acceptor实例的新连接回调函数，在后续有新连接时被调用
void TcpServer::onNewConnection(int sockfd, const InetAddress& peerAddr) {
    try {
        // 创建新TcpConnection实例
        TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, sockfd, listenAddr_, peerAddr);
        // 设置连接回调
        conn->setConnectionCallback([this](const TcpConnectionPtr& conn) { onConnection(conn); });
        // 设置消息接收回调
        conn->setMessageCallback(
            [this](const TcpConnectionPtr& conn, Buffer& buffer) { messageCallback_(conn, buffer); });
        // 设置写完成回调
        conn->setWriteCompleteCallback([this](const TcpConnectionPtr& conn) {
            if (writeCompleteCallback_) { writeCompleteCallback_(conn); }
        });
        // 设置关闭回调
        conn->setCloseCallback([this](const TcpConnectionPtr& conn) { removeConnection(conn); });

        // 在IO线程建立连接
        loop_->runInLoop([conn]() { conn->connectionEstablished(); });

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
        if (connectionCallback_) { connectionCallback_(conn); }
    } catch (const std::exception& e) { error("[TcpServer] onConnection() error: {}.\n\n", e.what()); }
}

// 移除连接
void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop([this, conn]() { removeConnectionInLoop(conn); });
}
// 在IO线程移除连接
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    assert(loop_->isInLoopThread());
    // 从map中移除
    connections_.erase(conn->fd());
    // 销毁连接
    loop_->queueInLoop([conn]() { conn->connectionDestroyed(); });
}