#pragma once

#include "socket/tcpSocketBase.h"

namespace net {
class Connector : public TcpSocketBase {
   public:
    Connector() : TcpSocketBase(){};

    Connector(int connFd) : TcpSocketBase(connFd){};

    Connector(Connector&& other) noexcept;

    Connector& operator=(Connector&& other) noexcept;

    ~Connector() override = default;

    bool connectTo(const std::string& ip, int port);

    int send(const void* data, int length);

    int recv(void* buffer, int length);

    int getErrorNo() const;
};
}  // namespace net