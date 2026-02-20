#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

#include "socket/InetAddress.h"

using net::InetAddress;

// 测试IPv4地址构造与获取地址信息方法
TEST(InetAddressTest, IPv4Address) {
    const std::string ip = "127.0.0.1";
    const int port = 8080;
    InetAddress addr(ip, port);

    EXPECT_EQ(addr.toIp(), ip);
    EXPECT_EQ(addr.toPort(), port);
    EXPECT_EQ(addr.toIpPort(), ip + ":" + std::to_string(port));
}

// 测试仅端口构造
TEST(InetAddressTest, PortOnlyConstructor) {
    const int port = 9999;
    InetAddress addr(port);

    EXPECT_EQ(addr.toIp(), "0.0.0.0");
    EXPECT_EQ(addr.toPort(), port);
    EXPECT_EQ(addr.toIpPort(), "0.0.0.0:" + std::to_string(port));
}

// 测试仅端口构造且在本地回环
TEST(InetAddressTest, PortOnlyConstructorWithLoopbackOnly) {
    const int port = 8888;
    InetAddress addr(port, true);

    EXPECT_EQ(addr.toIp(), "127.0.0.1");
    EXPECT_EQ(addr.toPort(), port);
    EXPECT_EQ(addr.toIpPort(), "127.0.0.1:" + std::to_string(port));
}

// 测试sockaddr_in构造
TEST(InetAddressTest, SockaddrConstructor) {
    const std::string ip = "192.168.1.1";
    const int port = 12345;

    struct sockaddr_in sock_addr{};
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = ::htons(port);
    ::inet_pton(AF_INET, ip.data(), &sock_addr.sin_addr);

    InetAddress addr(sock_addr);

    EXPECT_EQ(addr.toIp(), "192.168.1.1");
    EXPECT_EQ(addr.toPort(), port);
    EXPECT_EQ(addr.toIpPort(), "192.168.1.1:" + std::to_string(port));
}

// 测试端口边界情况
TEST(InetAddressTest, PortBoundaries) {
    // 测试最小有效端口
    InetAddress addr_min(1);
    EXPECT_EQ(addr_min.toIpPort(), "0.0.0.0:1");

    // 测试最大有效端口
    InetAddress addr_max(65535);
    EXPECT_EQ(addr_max.toIpPort(), "0.0.0.0:65535");
}

// 测试特殊IP地址
TEST(InetAddressTest, SpecialIpAddresses) {
    // 测试回环地址
    InetAddress loopback("127.0.0.1", 8888);
    EXPECT_EQ(loopback.toIpPort(), "127.0.0.1:8888");

    // 测试广播地址
    InetAddress broadcast("255.255.255.255", 8888);
    EXPECT_EQ(broadcast.toIpPort(), "255.255.255.255:8888");

    // 测试网络地址
    InetAddress network("0.0.0.0", 8888);
    EXPECT_EQ(network.toIpPort(), "0.0.0.0:8888");
}
