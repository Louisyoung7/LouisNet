#include "base/Buffer.h"
#include "log/Logger.h"
#include "net/InetAddress.h"
#include "net/TcpClient.h"
#include "net/TcpConnection.h"
#include "net/reactor/EventLoop.h"

using namespace net;
using namespace net::reactor;
using namespace base;
using TcpConnectionPtr = TcpConnection::TcpConnectionPtr;

int main() {
    // 创建EventLoop
    EventLoop loop;

    // 配置服务器地址
    InetAddress serverAddr("127.0.0.1", 8080);

    TcpClient client(&loop, serverAddr);

    logging::init();

    // 设置连接回调
    client.setConnectionCallback([](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            logging::info("Connected");
            conn->send("Hello, server!");
        } else {
            logging::info("Disconnected");
        }
    });

    // 设置消息回调
    client.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf) {
        logging::info("TcpClient received message: {}", buf.retrieveAllAsString());
        conn->send("Hello, server!");
        conn->shutdown();
        logging::info("TcpClient shutdown");
    });

    client.connect();

    logging::info("TcpClient connected to server at {}:", serverAddr.toIpPort());

    loop.loop();
}
