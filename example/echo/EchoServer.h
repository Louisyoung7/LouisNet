#pragma once

#include <memory>

#include "base/Buffer.h"
#include "base/ThreadPool.h"
#include "net/TcpServer.h"

namespace net::reactor {
class EventLoop;
}

namespace net {
class InetAddress;
}  // namespace net

class EchoServer {
    std::unique_ptr<net::TcpServer> server_;        ///< 组合模式包含TcpServer实例
    std::unique_ptr<base::ThreadPool> threadPool_;  ///< 线程池

   public:
    // 构造函数
    EchoServer(net::reactor::EventLoop* loop, const net::InetAddress& listenAddr, int numThreads);

    // 启动服务器
    void start();

   private:
    // 处理连接状态变化
    void onConnection(const net::TcpServer::TcpConnectionPtr& conn);

    // 处理消息接收
    void onMessage(const net::TcpServer::TcpConnectionPtr& conn, base::Buffer& buffer);
};