#include "log/Logger.h"
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
    InetAddress listenAddr(8080);

    // 创建HttpServer
    HttpServer server(&loop, listenAddr);

    // 注册路径对应的请求处理回调函数
    server.registerHandler("/", [](HttpContext& ctx) {
        auto& resp = ctx.response();

        // 构造echo内容
        resp.setVersion("HTTP/1.1")
            .setStatusCode(200)
            .setHeader("Content-Type", "text/plain")
            .setHeader("Content-Length", std::to_string(0))
            .setBody("");
    });

    // 启动服务器
    server.start();

    // 初始化日志系统
    init();

    // 打印服务器启动信息
    info("HttpServer started on port 8080");

    // 启动事件循环
    loop.loop();

    // 关闭spdlog，保证日志文件被写入
    shutdown();
    return 0;
}