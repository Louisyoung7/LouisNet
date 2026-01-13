#include "socket/listener.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "utils.hpp"  // for setReuseAddr
using std::string;
using std::to_string;

namespace net {
Listener& Listener::operator=(Listener&& other) noexcept {
    if (&other == this) {
        return *this;
    }
    TcpSocketBase::operator=(std::move(other));
    return *this;
}

bool Listener::create() {
    sockFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ == -1) {
        setError("socket failed: socket syscall error. " + string(::strerror(errno)));
        return false;
    }

    return true;
}

bool Listener::bindAndListen(const std::string ip, int port, int backLog) {
    if (!fdIsValid()) {
        setError("Invalid socket fd.");
        return false;
    }

    // 允许端口复用，避免出现端口正在被使用的报错
    utils::setReuseAddr(sockFd_);

    // 设置服务器地址信息
    struct sockaddr_in sockAddr {};
    sockAddr.sin_family = AF_INET;    ///< 地址族
    sockAddr.sin_port = htons(port);  ///< 端口，注意字节序转换

    // 指定ip，错误判断
    if (ip == to_string(INADDR_ANY)) {
        sockAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        int ptonRet = ::inet_pton(AF_INET, ip.c_str(), &sockAddr.sin_addr.s_addr);
        if (ptonRet != 1) {
            if (ptonRet == 0) {
                setError("bind failed: invalid IP address. " + string(::strerror(errno)));
            } else if (ptonRet == -1) {
                setError("bind failed: inet_pton call error. " + string(::strerror(errno)));
            }
            return false;
        }
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
        setError("Invalid socket fd.");
        return -1;  ///< 表示accept失败
    }
    int connFd = ::accept(sockFd_, nullptr, nullptr);
    if (connFd < 0) {
        setError("accept failed: accept syscall error. " + string(::strerror(errno)));
        errNo_ = errno;
        return -1;
    }

    return connFd;
}

int Listener::getErrNo() const {
    return errNo_;
}
}  // namespace net