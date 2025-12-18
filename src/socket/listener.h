#pragma once

#include <socket/tcpSocketBase.h>
#include <sys/socket.h>  // for SOMAXCONN
#include <arpa/inet.h>
#include <string>

namespace net {

/**
 * @brief 对监听套接字的封装
 */
class Listener : public TcpSocketBase {
   private:
   public:
    Listener() : TcpSocketBase(){};

    ~Listener() override = default;

    /**
     * @brief 绑定端口，设置监听
     */
    bool bindAndListen(const std::string ip, int port, int backLog);

    int accept();

    int getSockFd() const override;
};

}  // namespace net