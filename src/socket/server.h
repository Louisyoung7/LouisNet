#pragma once

#include <socket/socket.h>

namespace net {
class TcpServer : public TcpSocket {
   public:
    TcpServer();
    ~TcpServer();

    /**
     * @brief 开启服务器
     * 底层会调用socket，bind，listen函数
     * @return true 开启成功
     * @return false 开启失败
     */
    bool start(const std::string& ip, int port);
};
}  // namespace net