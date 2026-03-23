#include "InetAddress.h"

#include <arpa/inet.h>

#include <cstdint>

namespace net {
// 仅端口构造，可以指定是否本地回环
InetAddress::InetAddress(uint16_t port, bool loopbackOnly) {
    addr_.sin_family = AF_INET;
    // 监听回环地址还是监听所有地址
    in_addr_t ip = loopbackOnly ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
    addr_.sin_addr.s_addr = ip;
    addr_.sin_port = htons(port);
}
// IP，端口构造
InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}
// sockaddr_in构造
InetAddress::InetAddress(const struct sockaddr_in& addr) : addr_(addr) {
}

// 获取IP
std::string InetAddress::toIp() const {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
// 获取端口
uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}
// 获取IP:端口
std::string InetAddress::toIpPort() const {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    uint16_t port = ntohs(addr_.sin_port);
    return std::string(buf) + ":" + std::to_string(port);
}
}  // namespace net