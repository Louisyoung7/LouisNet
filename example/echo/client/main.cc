#include "echo.h"
#include "log/Logger.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"

using namespace net;
using namespace net::reactor;

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8080);
    EchoClient client(&loop, serverAddr);
    client.start();
    logging::info("start client");
    loop.loop();
    spdlog::drop_all();
    spdlog::shutdown();
    return 0;
}