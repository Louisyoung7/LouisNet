#include <socket/connector.h>
#include <arpa/inet.h>

namespace net {
bool Connector::connectTo(const std::string& ip, int port) {
    if (!fdIsValid()) {
        return false;
    }

    struct sockaddr_in commAddr {};
    commAddr.sin_family = AF_INET;
    commAddr.sin_port = htons(port);  ///< 指定端口号，注意字节序转换

    // 指定IP，错误判断
    int ptonRet = ::inet_pton(AF_INET, ip.c_str(), &commAddr.sin_addr.s_addr);
    if (ptonRet != 1) {
        if (ptonRet == 0) {
            setError("connect faield: invalid IP address.");
        } else if (ptonRet == -1) {
            setError("connect failed: inet_pton call error.");
        }
        return false;
    }

    if (::connect(sockFd_, reinterpret_cast<struct sockaddr*>(&commAddr), sizeof(commAddr)) == -1) {
        setError("connect failed: connect syscall error.");
        return false;
    }

    return true;
}
}