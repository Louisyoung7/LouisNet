#include "HttpServer.h"

#include "HttpContext.h"
#include "HttpParser.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"

using namespace net::http;
using namespace net::reactor;
using namespace net;
using namespace base;

struct HttpServer::HttpServerContext {
    HttpContext ctx;          // 上下文对象
    HttpParser parser;        // HTTP解析器对象
    bool responded_ = false;  // 是否响应过，HTTP协议中每个请求只能响应一次
    HttpServerContext(TcpConnection* conn) : ctx(conn) {}
};

HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(std::make_unique<TcpServer>(loop, listenAddr)) {
    // 设置连接状态回调
    server_->setConnectionCallback([this](const TcpServer::TcpConnectionPtr& conn) { onConnection(conn); });

    // 设置消息接收回调
    server_->setMessageCallback(
        [this](const TcpServer::TcpConnectionPtr& conn, Buffer& buffer) { onMessage(conn, buffer); });
}

void HttpServer::start() { server_->start(); }

void HttpServer::onConnection(const TcpServer::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        // 连接建立时创建上下文
        conn->setContext(std::make_shared<HttpServerContext>(conn.get()));
    } else {
        // 连接断开时移除上下文
        conn->setContext(nullptr);
    }
}

void HttpServer::onMessage(const TcpServer::TcpConnectionPtr& conn, Buffer& buffer) {
    
    
}