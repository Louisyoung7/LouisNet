#include <socket/listener.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <string>
using std::string;

namespace net {
bool Listener::bindAndListen(const std::string ip, int port, int backLog) {
    if (fdIsValid()) {
        setError("Invalid socket fd.");
        return false;
    }

    // 设置服务器地址信息
    struct sockaddr_in sockAddr {};
    sockAddr.sin_family = AF_INET;  ///< 地址族
    sockAddr.sin_port = htons(port);  ///< 端口，注意字节序转换

    // 指定ip，错误判断
    int ptonRet = ::inet_pton(AF_INET, ip.c_str(), &sockAddr.sin_addr.s_addr);
    if (ptonRet != 1) {
        if (ptonRet == 0) {
            setError("bind failed: invalid IP address. " + string(::strerror(errno)));
        } else if (ptonRet == -1) {
            setError("bind failed: inet_pton call error. " + string(::strerror(errno)));
        }
        return false;
    }

    if (::bind(sockFd_, reinterpret_cast<struct sockaddr*>(&sockAddr), sizeof(sockAddr)) == -1) {
        setError("bind failed: bind syscall error. " + string(::strerror(errno)));
        return false;
    }

    if (::listen(sockFd_, backLog) == -1) {
        setError("listen failed: listen syscall error. " + string(::strerror(errno)));
        return false;
    }

    return true;
}

int Listener::accept() {
    if (!fdIsValid()) {
        return -1;  ///< 表示accept失败
    }
    int connFd = ::accept(sockFd_, nullptr, nullptr);
    if (connFd < 0) {
        setError("accept failed: accept syscall error. " + string(::strerror(errno)));
        return -1;
    }

    return connFd;
}
}