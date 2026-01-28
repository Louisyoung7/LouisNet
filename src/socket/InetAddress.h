#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <string>

namespace net {
class InetAddress {
   public:
    // 默认为0，表示让操作系统自动分配一个可用端口
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);
    InetAddress(const std::string& ip, uint16_t port);
    explicit InetAddress(const struct sockaddr_in& addr);
    
    // 获取地址
    std::string toIp() const;
    uint16_t toPort() const;
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
