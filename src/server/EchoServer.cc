#include "EchoServer.h"

#include <iostream>
#include <memory>
#include <string>

using std::cerr;
using std::cout;
using std::endl;

namespace server {
// 构造函数
EchoServer::EchoServer(reactor::EventLoop* loop, const net::InetAddress& listenAddr)
    : server_(std::make_unique<TcpServer>(loop, listenAddr)) {
    // 设置消息接收回调
    server_->setMessageCallback(
        [this](const TcpServer::TcpConnectionPtr& conn, utils::Buffer& buffer) { onMessage(conn, buffer); });
    
    // 设置连接状态回调
    server_->setConnectionCallback([this](const TcpServer::TcpConnectionPtr& conn) { onConnection(conn); });
}

// 启动服务器
void EchoServer::start() {
    try {
        cout << "[EchoServer] start() starting EchoServer on " << server_->listenAddr().toIpPort() << endl << endl;

        server_->start();
    } catch (const std::exception& e) {
        cerr << "[EchoServer] start() error: " << e.what() << endl << endl;
    }
}

// 处理连接状态变化
// 被设置为TcpServer的连接状态回调，在连接状态变化时输出日志
void EchoServer::onConnection(const TcpServer::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        cout << "[EchoServer] onConnection() connection " << conn->name() << " established" << endl << endl;
    } else {
        cout << "[EchoServer] onConnection() connection " << conn->name() << " disconnected" << endl << endl;
    }
}

// 处理消息接收
// 被设置为TcpServer的消息接收回调，调用连接实例的send方法回显数据
void EchoServer::onMessage(const TcpServer::TcpConnectionPtr& conn, utils::Buffer& buffer) {
    try {
        // 读取buffer中的所有可读数据到string
        std::string message = buffer.retrieveAllAsString();

        cout << "[EchoServer] onMessage() connection " << conn->name() << " received " << message.size()
             << " bytes: " << message << endl
             << endl;
        
        // 回显数据
        conn->send(message);
    } catch (const std::exception& e) {
        cerr << "[EchoServer] onMessage() error: " << e.what() << endl << endl;
        return;
    }
}
}  // namespace server