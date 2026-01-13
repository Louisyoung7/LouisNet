#pragma once

#include "socket/connector.h"
#include "socket/listener.h"

namespace net {
class TcpServer {
   private:
    Listener listener_;

   public:
    TcpServer() = default;
    ~TcpServer() = default;

    /**
     * @brief 开启服务器
     */
    bool start(const std::string& ip = std::to_string(INADDR_ANY), int port = 8080, int backlog = SOMAXCONN);

    int getListenFd() const;

    int accept();
};
}  // namespace net