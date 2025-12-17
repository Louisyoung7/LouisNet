#pragma once

#include <socket/tcpSocketBase.h>

namespace net {
class Connector : public TcpSocketBase {
   private:
    int errNo;  ///< 存储errno
   public:
    Connector() : TcpSocketBase(){};

    ~Connector() override = default;

    bool connectTo(const std::string& ip, int port);

    int send(const void* data, int length);

    int recv(void* buffer, int length);
};
}  // namespace net