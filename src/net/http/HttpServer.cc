#include "HttpServer.h"

#include "HttpContext.h"
#include "HttpParser.h"
#include "net/InetAddress.h"
#include "net/http/HttpRequest.h"
#include "net/reactor/EventLoop.h"

using namespace net::http;
using namespace net::reactor;
using namespace net;
using namespace base;

struct HttpServer::HttpServerContext {
    HttpContext ctx;    // 上下文对象
    HttpParser parser;  // HTTP解析器对象
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

    while (buffer.readableBytes() > 0) {
        auto result = HttpCtx->parser.parseRequest(buffer, HttpCtx->ctx.request());
        if (result == HttpParser::ParseResult::kSuccess) {
            // 解析成功，调用注册的请求处理回调函数
            onRequest(HttpCtx->ctx);

            // 判断是否Keep-Alive
            bool keepAlive = isKeepAlive(HttpCtx->ctx.request());
            HttpCtx->ctx.response().setHeader("Connection", keepAlive ? "keep-alive" : "close");

            // 发送响应
            std::string responseStr = HttpCtx->ctx.response().toString();

            // 重置上下文
            resetContext(*HttpCtx.get());

            // 发送响应
            conn->send(responseStr);

            if (!keepAlive) { conn->shutdown(); }
        } else if (result == HttpParser::ParseResult::kError) {
            // 解析失败，返回400错误
            HttpCtx->ctx.response().setStatusCode(400).setHeader("Connection", "close").setBody("Bad Request");

            std::string responseStr = HttpCtx->ctx.response().toString();

            // 重置上下文
            resetContext(*HttpCtx.get());

            // 发送响应
            conn->send(responseStr);

            conn->shutdown();
        }
    }
}

// 处理请求
void HttpServer::onRequest(HttpContext& ctx) {
    // 获取请求路径
    const auto& req = ctx.request();
    auto it = handlers_.find(req.path_);
    if (it == handlers_.end()) {
        // 没有注册该路径的回调函数，返回404错误
        ctx.response()
            .setVersion(req.version_)
            .setStatusCode(404)
            .setBody("Not Found")
            .setHeader("Content-Length", "10");
        return;
    }

    // 调用注册的回调函数
    it->second(ctx);
}

// 判断是否Keep-Alive
bool HttpServer::isKeepAlive(const HttpRequest& req) {
    auto it = req.headers_.find("connection");

    // HTTP/1.1协议中，默认是Keep-Alive，除非指定为close
    // HTTP/1.0协议中，默认是close，除非指定为keep-alive
    if (req.version_ == "HTTP/1.1") {
        // 没有connection头，默认是Keep-Alive
        if (it == req.headers_.end()) { return true; }
        if (it->second == "close") { return false; }
        return true;
    } else {
        // 没有connection头，默认是close
        if (it == req.headers_.end()) { return false; }
        if (it->second == "keep-alive") { return true; }
        return false;
    }
}

// 重置上下文
void HttpServer::resetContext(HttpServerContext& ctx) {
    ctx.parser.reset();
    ctx.ctx.request().reset();
    ctx.ctx.response().reset();
}
