#pragma once

#include <memory>

#include "base/Buffer.h"
#include "net/TcpServer.h"

namespace net {
class InetAddress;

namespace reactor {
class EventLoop;
}
}  // namespace net

namespace net::http {
class HttpServer {
   public:
    HttpServer(net::reactor::EventLoop* loop, const net::InetAddress& listenAddr);

    void start();

   private:
    struct HttpServerContext;  ///< 连接上下文结构体，包含HttpContext和HttpParser

    std::unique_ptr<TcpServer> server_;  ///< 组合模式包含TcpServer实例

    // 处理连接状态变化
    void onConnection(const net::TcpServer::TcpConnectionPtr& conn);

    // 处理消息接收
    void onMessage(const net::TcpServer::TcpConnectionPtr& conn, base::Buffer& buffer);
};
}  // namespace net::http