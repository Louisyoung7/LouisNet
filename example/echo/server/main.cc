#include "echo.h"
#include "log/Logger.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"

using namespace net;
using namespace net::reactor;

int main() {
    // 创建EventLoop
    EventLoop loop;

    // 配置本端地址
    InetAddress listenAddr(8080);

    // 创建EchoServer
    EchoServer server(&loop, listenAddr);

    // 启动服务器
    server.start();

    // 初始化日志系统
    init();

    info("EchoServer started on port 8080");

    // 启动事件循环
    loop.loop();

    shutdown();
    return 0;
}