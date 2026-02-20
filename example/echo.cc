#include <iostream>

#include "EchoServer.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"

using std::cout;
using std::endl;

int main() {
    // 创建EventLoop
    net::reactor::EventLoop loop;

    // 配置本端地址
    net::InetAddress listenAddr(8888);

    // 创建EchoServer
    EchoServer server(&loop, listenAddr);

    // 启动服务器
    server.start();

    cout << "EchoServer started on port 8888" << endl << endl;

    // 启动事件循环
    loop.loop();
}