#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>  // for SOMAXCONN

#include <string>

#include "socket/tcpSocketBase.h"

namespace net {
/**
 * @brief 对监听套接字的封装
 */
class Listener : public TcpSocketBase {
   public:
    Listener() : TcpSocketBase(){};

    Listener(Listener&& other) noexcept : TcpSocketBase(std::move(other)){};

    Listener& operator=(Listener&& other) noexcept;

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

    /**
     * @brief 获取最新的errno
     */
    int getErrNo() const;
};

}  // namespace net