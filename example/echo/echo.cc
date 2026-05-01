#include "EchoServer.h"
#include "log/Logger.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"


int main() {
    // 创建EventLoop
    net::reactor::EventLoop loop;

    // 配置本端地址
    net::InetAddress listenAddr(8080);

    // 创建EchoServer，指定线程数为4
    EchoServer server(&loop, listenAddr, 4);

    // 启动服务器
    server.start();
    
    // 初始化日志系统
    logging::init();

    logging::info("EchoServer started on port 8080");

    // 启动事件循环
    loop.loop();
}