#pragma once

#include <arpa/inet.h>
#include <socket/tcpSocketBase.h>
#include <sys/socket.h>  // for SOMAXCONN

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
     * @brief 创建监听套接字
     */
    bool create();

    /**
     * @brief 绑定端口，设置监听
     */
    bool bindAndListen(const std::string ip, int port, int backLog);

    /**
     * @brief 接收连接请求
     */
    int accept();
};

}  // namespace net