#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <string>

namespace net {
class InetAddress {
   public:
    // 仅端口构造，可以指定是否本地回环
    explicit InetAddress(uint16_t port = 0,
                         bool loopbackOnly = false);  // port默认为0，表示让操作系统自动分配一个可用端口
    // IP，端口构造
    InetAddress(const std::string& ip, uint16_t port);
    // sockaddr_in构造
    explicit InetAddress(const struct sockaddr_in& addr);

    // 获取IP
    std::string toIp() const;
    // 获取端口
    uint16_t toPort() const;
    // 获取IP:端口
    std::string toIpPort() const;

    // 获取原始的sockaddr指针
    const struct sockaddr* getSockaddr() const {
        return reinterpret_cast<const struct sockaddr*>(&addr_);
    }
    // 设置地址
    void setSockAddrInet(const struct sockaddr_in& addr) {
        addr_ = addr;
    }

   private:
    struct sockaddr_in addr_ {};
};
}  // namespace net
