#include <socket/connector.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <cerrno>
using std::string;

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
            setError("connect faield: invalid IP address. " + string(::strerror(errno)));
        } else if (ptonRet == -1) {
            setError("connect failed: inet_pton call error. " + string(::strerror(errno)));
        }
        return false;
    }

    if (::connect(sockFd_, reinterpret_cast<struct sockaddr*>(&commAddr), sizeof(commAddr)) == -1) {
        setError("connect failed: connect syscall error. " + string(::strerror(errno)));
        return false;
    }

    return true;
}

int Connector::send(const void* data, int length) {
    ssize_t sendSize = ::send(sockFd_, data, length, 0);

    if (sendSize == -1) {
        errNo = errno;
    }
    return sendSize;
}

int Connector::recv(void* buffer, int length) {
    ssize_t recvSize = ::recv(sockFd_, buffer, length, 0);

    if (recvSize == -1) {
        errNo = errno;
    }
    return recvSize;
}
}