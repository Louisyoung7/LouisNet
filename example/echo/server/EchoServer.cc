#include "EchoServer.h"

#include <memory>
#include <string>

#include "log/Logger.h"

using namespace net;
using namespace net::reactor;
using namespace base;

// 构造函数
EchoServer::EchoServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(std::make_unique<TcpServer>(loop, listenAddr)) {
    // 设置消息接收回调
    server_->setMessageCallback(
        [this](const TcpServer::TcpConnectionPtr& conn, Buffer& buffer) { onMessage(conn, buffer); });

    // 设置连接状态回调
    server_->setConnectionCallback([this](const TcpServer::TcpConnectionPtr& conn) { onConnection(conn); });
}

// 启动服务器
void EchoServer::start() {
    try {
        server_->start();
    } catch (const std::exception& e) { logging::error("[EchoServer] start() error: {}.\n\n", e.what()); }
}

// 处理连接状态变化
// 被设置为TcpServer的连接状态回调，在连接状态变化时输出日志
void EchoServer::onConnection(const TcpServer::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        logging::debug("[EchoServer] onConnection() connection {} established.\n\n", conn->name());
    } else {
        logging::debug("[EchoServer] onConnection() connection {} disconnected.\n\n", conn->name());
    }
}

// 处理消息接收
// 被设置为TcpServer的消息接收回调，调用连接实例的send方法回显数据
void EchoServer::onMessage(const TcpServer::TcpConnectionPtr& conn, Buffer& buffer) {
    try {
        // 读取buffer中的所有可读数据到string
        std::string message = buffer.retrieveAllAsString();

        logging::info("[EchoServer] onMessage() connection {} received {} bytes: {}.\n\n", conn->name(), message.size(),
                      message.c_str());

        // 回显数据
        conn->send(message);
    } catch (const std::exception& e) {
        logging::error("[EchoServer] onMessage() error: {}.\n\n", e.what());
        return;
    }
}