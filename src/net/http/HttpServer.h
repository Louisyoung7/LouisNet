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
class HttpContext;

class HttpServer {
   public:
    using RequestHandler = std::function<void(HttpContext&)>;

    HttpServer(net::reactor::EventLoop* loop, const net::InetAddress& listenAddr);

    void start();

    // 设置请求处理回调函数
    void setRequestHandler(RequestHandler cb) { requestHandler_ = std::move(cb); }

   private:
    struct HttpServerContext;  ///< 连接上下文结构体，包含HttpContext和HttpParser

    std::unique_ptr<TcpServer> server_;  ///< 组合模式包含TcpServer实例
    RequestHandler requestHandler_;      ///< 请求处理回调函数

    // 处理连接状态变化
    void onConnection(const net::TcpServer::TcpConnectionPtr& conn);

    // 处理消息接收
    void onMessage(const net::TcpServer::TcpConnectionPtr& conn, base::Buffer& buffer);

    // 判断是否Keep-Alive
    bool isKeepAlive(const HttpContext& ctx);
};
}  // namespace net::http