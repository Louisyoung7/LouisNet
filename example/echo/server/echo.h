#pragma once

#include "base/Buffer.h"
#include "net/TcpConnection.h"
#include "net/TcpServer.h"

class EchoServer {
   public:
    EchoServer(net::reactor::EventLoop* loop, const net::InetAddress& listenAddr);

    void start();

   private:
    net::TcpServer server_;
    net::reactor::EventLoop* loop_;

    void onConnection(const net::TcpConnectionPtr& conn);
    void onMessage(const net::TcpConnectionPtr& conn, base::Buffer& buffer);
};