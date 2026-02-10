#pragma once

#include <memory>

#include "TcpServer.h"
#include "utils/Buffer.h"

namespace reactor {
class EventLoop;
}

namespace net {
class InetAddress;
}

namespace server {
class EchoServer {
   public:
    // 构造函数
    EchoServer(reactor::EventLoop* loop, const net::InetAddress& listenAddr);

    // 启动服务器
    void start();

   private:
    // 处理连接状态变化
    void onConnection(const TcpServer::TcpConnectionPtr& conn);

    // 处理消息接收
    void onMessage(const TcpServer::TcpConnectionPtr& conn, utils::Buffer& buffer);
    
    std::unique_ptr<TcpServer> server_;  ///< 组合模式包含TcpServer实例
};
}  // namespace server