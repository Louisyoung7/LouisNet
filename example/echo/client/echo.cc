#include "echo.h"

#include "base/Buffer.h"
#include "log/Logger.h"
#include "net/reactor/EventLoop.h"

using namespace net;
using namespace net::reactor;
using namespace base;

EchoClient::EchoClient(EventLoop* loop, const InetAddress& serverAddr)
    : client_(loop, serverAddr), serverAddr_(serverAddr), loop_(loop) {
    client_.setConnectionCallback([this](const TcpConnectionPtr& conn) { onConnection(conn); });
    client_.setMessageCallback([this](const TcpConnectionPtr& conn, Buffer& buffer) { onMessage(conn, buffer); });
}

void EchoClient::start() { client_.connect(); }

void EchoClient::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) { conn->send("hello world"); }
}
void EchoClient::onMessage([[maybe_unused]] const TcpConnectionPtr& conn, Buffer& buffer) {
    std::string msg = buffer.retrieveAllAsString();
    info("onMessage: {}", msg);
}
