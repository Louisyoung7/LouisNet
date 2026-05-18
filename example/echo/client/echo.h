#pragma once

#include "net/InetAddress.h"
#include "net/TcpClient.h"

class EchoClient {
   public:
    EchoClient(net::reactor::EventLoop* loop, const net::InetAddress& serverAddr);

    void start();

   private:
    net::TcpClient client_;
    net::InetAddress serverAddr_;
    net::reactor::EventLoop* loop_;

    void onConnection(const net::TcpConnectionPtr& conn);
    void onMessage(const net::TcpConnectionPtr& conn, base::Buffer& buffer);
};