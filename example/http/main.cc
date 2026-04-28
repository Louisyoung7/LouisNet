#include <iostream>

#include "net/Acceptor.h"
#include "net/InetAddress.h"
#include "net/http/HttpContext.h"
#include "net/http/HttpServer.h"
#include "net/reactor/EventLoop.h"
using namespace std;
using namespace net;
using namespace net::http;
using namespace net::reactor;

int main() {
    // 创建EventLoop
    EventLoop loop;

    // 配置本端地址
    InetAddress listenAddr(8888);

    // 创建HttpServer
    HttpServer server(&loop, listenAddr);

    // 设置请求处理回调
    server.setRequestHandler([](HttpContext& ctx) {
        const auto& req = ctx.request();
        auto& resp = ctx.response();

        // 构造echo内容
        std::string echo = "Method: " + std::string(req.method_) + "\r\n" + "Path: " + std::string(req.path_) + "\r\n" +
                           "Headers:\r\n";
        for (const auto& [key, value] : req.headers_) {
            echo += key + ": " + value + "\r\n";
        }

        if (!req.body_.empty()) {
            echo += "Body:\r\n" + req.body_ + "\r\n";
        }

        resp.setVersion(req.version_)
            .setStatusCode(200)
            .setHeader("Content-Type", "text/plain")
            .setHeader("Content-Length", std::to_string(echo.size()))
            .setBody(echo);
    });

    // 启动服务器
    server.start();

    // 打印服务器启动信息
    cout << "HttpServer started on port 8888" << endl << endl;

    // 启动事件循环
    loop.loop();
}