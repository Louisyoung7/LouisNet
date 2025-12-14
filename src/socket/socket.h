#pragma once

namespace net {
class TcpSocket {
   protected:
    int sockFd_;

   protected:
    void create();
    void bind();
    void listen();
    void accept();
    void connect();

   public:
    TcpSocket() : sockFd_(-1){};
    virtual ~TcpSocket();
    virtual void send();
    virtual void recv();
};
}  // namespace net