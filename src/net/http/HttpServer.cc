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
    // 获取上下文对象
    auto HttpCtx = std::any_cast<std::shared_ptr<HttpServerContext>>(conn->getContext());
    if (!HttpCtx) {
        // 连接上下文不存在，直接返回
        return;
    }

    if (HttpCtx->responded_) {
        // 已响应过，直接返回
        return;
    }

    auto result = HttpCtx->parser.parseRequest(buffer, HttpCtx->ctx.request());
    if (result == HttpParser::ParseResult::kSuccess) {
        // 解析成功，调用请求处理回调
        requestHandler_(HttpCtx->ctx);

        // 响应完成，设置响应过标志
        HttpCtx->responded_ = true;

        // 发送响应
        std::string responseStr = HttpCtx->ctx.response().toString();
        conn->send(responseStr);
        conn->shutdown();
    } else if (result == HttpParser::ParseResult::kError) {
        // 解析失败，返回400错误
        HttpCtx->ctx.response().setStatusCode(400).setBody("Bad Request");

        // 设置响应过标志
        HttpCtx->responded_ = true;

        std::string responseStr = HttpCtx->ctx.response().toString();
        conn->send(responseStr);
        conn->shutdown();
    }
}