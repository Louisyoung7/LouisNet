#include "TcpServer.h"

#include <unistd.h>

#include <iostream>
#include <memory>

#include "socket/Acceptor.h"
#include "socket/TcpConnection.h"

using std::cerr;
using std::cout;
using std::endl;

namespace server {
// 构造函数
// 初始化TcpServer，并设置Acceptor实例的新连接回调函数
TcpServer::TcpServer(reactor::EventLoop* loop, const net::InetAddress& listenAddr)
    : loop_(loop),
      listenAddr_(listenAddr),
      name_("TcpServer@" + listenAddr.toIpPort()),
      acceptor_(std::make_unique<net::Acceptor>(loop, listenAddr)) {
    // 设置Acceptor实例的新连接回调函数
    // 新连接回调函数需要Acceptor获取的通信套接字和对端地址作为参数
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const net::InetAddress& peerAddr) { onNewConnection(sockfd, peerAddr); });
}

// 析构函数
// 将仍存储在TcpServer的所有TcpConnection连接实例全部优雅关闭
TcpServer::~TcpServer() {
    for (auto& conn : connections_) {
        // 使用shutdown半关闭
        conn.second->shutdown();
    }
}

// 启动服务器
// 让Acceptor实例开始监听新连接
void TcpServer::start() {
    cout << "[TcpServer] start() starting to listen on " << listenAddr_.toIpPort() << endl << endl;
    acceptor_->listen();
}

// 处理新连接
// 会被设置为Acceptor实例的新连接回调函数，在后续有新连接时被调用
void TcpServer::onNewConnection(int sockfd, const net::InetAddress& peerAddr) {
    cout << "[TcpServer] onNewConnection() new connection from " << peerAddr.toIpPort() << " sockfd = " << sockfd
         << endl
         << endl;

    try {
        // 创建新TcpConnection实例
        TcpConnectionPtr conn = std::make_shared<net::TcpConnection>(loop_, sockfd, listenAddr_, peerAddr);
        // 设置连接回调
        conn->setConnectionCallback([this](const TcpConnectionPtr& conn) { onConnection(conn); });
        // 设置消息接收回调
        conn->setMessageCallback(
            [this](const TcpConnectionPtr& conn, utils::Buffer& buffer) { messageCallback_(conn, buffer); });
        // 设置写完成回调
        conn->setWriteCompleteCallback([this](const TcpConnectionPtr& conn) {
            if (writeCompleteCallback_) {
                writeCompleteCallback_(conn);
            }
        });
        // 设置关闭回调
        conn->setCloseCallback([this](const TcpConnectionPtr& conn) { onClose(conn); });

        // 建立连接
        conn->connectionEstablished();

        // 存储实例
        connections_[sockfd] = conn;
    } catch (const std::exception& e) {
        cerr << "[TcpServer] onNewConnection() error: " << e.what() << endl << endl;
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
        cerr << "[TcpServer] onConnection() error: " << e.what() << endl << endl;
    }
}
// 处理关闭事件
// 在onNewConnection中被设置为新创建的TcpConnection的回调函数
void TcpServer::onClose(const TcpConnectionPtr& conn) {
    try {
        cout << "[TcpServer] onClose() connection " << conn->name() << " closed" << endl << endl;
        // 从map中移除连接记录
        // 后续由TcpConnection自动管理生命周期
        connections_.erase(conn->fd());
    } catch (const std::exception& e) {
        cerr << "[TcpServer] onClose() error: " << e.what() << endl << endl;
    }
}
}  // namespace server