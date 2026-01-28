#include "InetAddress.h"

#include <arpa/inet.h>
#include <cstdint>

namespace net {
InetAddress::InetAddress(uint16_t port, bool loopbackOnly) {
    addr_.sin_family = AF_INET;
    // 监听回环地址还是监听所有地址
    in_addr_t ip = loopbackOnly ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
    addr_.sin_addr.s_addr = ip;
    addr_.sin_port = htons(port);
}
InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}
InetAddress::InetAddress(const struct sockaddr_in& addr) : addr_(addr) {
}

// 获取地址
std::string InetAddress::toIp() const {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}
std::string InetAddress::toIpPort() const {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    uint16_t port = ntohs(addr_.sin_port);
    return std::string(buf) + ":" + std::to_string(port);
}
}  // namespace net