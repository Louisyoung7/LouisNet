#include "echo.h"

#include <string>

#include "log/Logger.h"

using namespace net;
using namespace net::reactor;
using namespace base;

EchoServer::EchoServer(EventLoop* loop, const InetAddress& listenAddr) : server_(loop, listenAddr), loop_(loop) {
    // 设置消息接收回调
    server_.setMessageCallback([this](const TcpConnectionPtr& conn, Buffer& buffer) { onMessage(conn, buffer); });

    // 设置连接状态回调
    server_.setConnectionCallback([this](const TcpConnectionPtr& conn) { onConnection(conn); });
}

void EchoServer::start() { server_.start(); }

// 处理连接状态变化
// 被设置为TcpServer的连接状态回调，在连接状态变化时输出日志
void EchoServer::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        debug("[EchoServer] onConnection() connection {} established.\n\n", conn->name());
    } else {
        debug("[EchoServer] onConnection() connection {} disconnected.\n\n", conn->name());
    }
}

// 处理消息接收
// 被设置为TcpServer的消息接收回调，调用连接实例的send方法回显数据
void EchoServer::onMessage(const TcpConnectionPtr& conn, Buffer& buffer) {
    // 读取buffer中的所有可读数据到string
    std::string message = buffer.retrieveAllAsString();

    info("[EchoServer] onMessage() connection {} received {} bytes: {}.\n\n", conn->name(), message.size(),
         message.c_str());

    // 回显数据
    conn->send(message);
}