#pragma once

#include <memory>
#include <unordered_map>

#include "base/Buffer.h"
#include "net/TcpServer.h"
#include "net/http/HttpParser.h"

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

    // 注册路径对应的请求处理回调函数
    void registerHandler(const std::string& path, RequestHandler cb) { handlers_[path] = std::move(cb); }

   private:
    struct HttpServerContext;  ///< 连接上下文结构体，包含HttpContext和HttpParser

    std::unique_ptr<TcpServer> server_;                         ///< 组合模式包含TcpServer实例
    std::unordered_map<std::string, RequestHandler> handlers_;  ///< 路径对应的请求处理回调函数映射

    // 处理连接状态变化
    void onConnection(const TcpConnectionPtr& conn);

    // 处理消息接收
    void onMessage(const TcpConnectionPtr& conn, base::Buffer& buffer);

    // 处理请求
    void onRequest(HttpContext& ctx);

    // 判断是否Keep-Alive
    bool isKeepAlive(const HttpRequest& req);

    // 重置上下文
    void resetContext(HttpServerContext& ctx);
};
}  // namespace net::http