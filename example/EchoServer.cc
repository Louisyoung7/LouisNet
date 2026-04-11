#include "EchoServer.h"

#include <memory>
#include <string>
#include <chrono>

#include "base/LouisLog.h"

using namespace net;
using namespace net::reactor;
using namespace base;

// 构造函数
EchoServer::EchoServer(EventLoop* loop, const InetAddress& listenAddr, int numThreads)
    : server_(std::make_unique<TcpServer>(loop, listenAddr)), threadPool_(std::make_unique<ThreadPool>(numThreads)) {
    // 设置消息接收回调
    server_->setMessageCallback(
        [this](const TcpServer::TcpConnectionPtr& conn, Buffer& buffer) { onMessage(conn, buffer); });

    // 设置连接状态回调
    server_->setConnectionCallback([this](const TcpServer::TcpConnectionPtr& conn) { onConnection(conn); });
}

// 启动服务器
void EchoServer::start() {
    try {
        INFO_F("[EchoServer] start() starting EchoServer on %s.\n\n", server_->listenAddr().toIpPort().c_str());

        server_->start();
    } catch (const std::exception& e) {
        ERROR_F("[EchoServer] start() error: %s.\n\n", e.what());
    }
}

// 处理连接状态变化
// 被设置为TcpServer的连接状态回调，在连接状态变化时输出日志
void EchoServer::onConnection(const TcpServer::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        INFO_F("[EchoServer] onConnection() connection %s established.\n\n", conn->name().c_str());
    } else {
        INFO_F("[EchoServer] onConnection() connection %s disconnected.\n\n", conn->name().c_str());
    }
}

// 处理消息接收
// 被设置为TcpServer的消息接收回调，调用连接实例的send方法回显数据
void EchoServer::onMessage(const TcpServer::TcpConnectionPtr& conn, Buffer& buffer) {
    try {
        // 读取buffer中的所有可读数据到string
        std::string message = buffer.retrieveAllAsString();

        INFO_F("[EchoServer] onMessage() connection %s received %ld bytes: %s.\n\n", conn->name().c_str(),
               message.size(), message.c_str());

        // 回显数据
        // 这里使用ThreadPool来异步发送数据，避免阻塞EventLoop线程
        threadPool_->submit([conn, message]() {
            // 模拟耗时操作，如数据库查询、文件写入等
            std::this_thread::sleep_for(std::chrono::seconds(1));
            conn->getLoop()->runInLoop([conn, message]() { conn->send(message); });
        });
    } catch (const std::exception& e) {
        ERROR_F("[EchoServer] onMessage() error: %s.\n\n", e.what());
        return;
    }
}