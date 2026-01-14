#pragma once

#include <arpa/inet.h>

#include <string>

namespace net {
class InetAddress {
   public:
    // 构造析构
    InetAddress(std::string ip, int port);
    InetAddress(const struct sockaddr_in& addr);
    ~InetAddress();

    // 获取IP地址
    std::string ip() const;
    // 获取端口号
    int port() const;
    // 获取struct sockaddr_in指针
    const sockaddr_in* sockaddr() const;

   private:
    struct sockaddr_in addr_ {};
};
}  // namespace net
