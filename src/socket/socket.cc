#include "socket/socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
using std::cerr;
using std::endl;
using std::string;

namespace net {

bool TcpSocket::start(const string& ip, int port) {
    // 创建套接字
    sockFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ == -1) {
        cerr << "socket failed: " << ::strerror(lastError) << endl;
        return false;
    }

    // 绑定端口
    if (!bind(ip, port)) {
        cerr << "bind failed: " << ::strerror(lastError) << endl;
        ::close(sockFd_);
        return false;
    }

    // 设置监听
    if (!listen()) {
        cerr << "listen failed: " << ::strerror(lastError) << endl;
        ::close(sockFd_);
        return false;
    }

    return true;
}

bool TcpSocket::bind(const string& ip, int port) {
    if (!sockFdIsValid()) {
        return false;
    }

    // 设置服务器地址信息
    struct sockaddr_in sockAddr {};
    sockAddr.sin_family = AF_INET;  ///< 地址族

    // 指定ip，错误判断
    int ptonRet = ::inet_pton(AF_INET, ip.c_str(), &sockAddr.sin_addr.s_addr);
    if (ptonRet != 1) {
        if (ptonRet == 0) {
            cerr << "Invalid IP address." << endl;
            lastError = errno;
        } else if (ptonRet == -1) {
            cerr << "inet_pton call failed" << endl;
            lastError = errno;
        }
        return false;
    }

    sockAddr.sin_port = htons(port);  ///< 端口，注意字节序转换

    if (::bind(sockFd_, reinterpret_cast<struct sockaddr*>(&sockAddr), sizeof(sockAddr)) == -1) {
        lastError = errno;
        return false;
    }

    return true;
}

bool TcpSocket::listen() {
    if (!sockFdIsValid()) {
        return false;
    }

    if (::listen(sockFd_, SOMAXCONN) == -1) {
        lastError = errno;
        return false;
    }

    return true;
}

int TcpSocket::accept() {
    if (!sockFdIsValid()) {
        return -1;  ///< 表示accept失败
    }
    int connFd = ::accept(sockFd_, nullptr, nullptr);
    if (connFd < 0) {
        lastError = errno;
        return -1;
    }

    return connFd;
}
bool TcpSocket::connect(const string& ip, int port) {
    if (!sockFdIsValid()) {
        return false;  
    }

    struct sockaddr_in commAddr{};
    commAddr.sin_family = AF_INET;
    int ptonRet = ::inet_pton(AF_INET, ip.c_str(), &commAddr.sin_addr.s_addr);
    if (ptonRet != 1) {
        if (ptonRet == 0) {
            cerr << "Invalid IP address." << endl;
        } else if (ptonRet == -1) {
            cerr << "inet_pton call failed" << endl;
            lastError = errno;
        }
        return false;
    }
    commAddr.sin_port = htons(port);

    if (::connect(sockFd_, reinterpret_cast<struct sockaddr*>(&commAddr), sizeof(commAddr)) == -1) {
        lastError = errno;
        return false;
    }

    return true;
}

int TcpSocket::getErrno() const {
    return lastError;
}

bool TcpSocket::sockFdIsValid() const {
    if (sockFd_ < 0) {
        cerr << "Invalid socket." << endl;
        return false;
    }

    return true;
}

TcpSocket::~TcpSocket() {
    if (sockFd_ > 0) {
        ::close(sockFd_);
    }
}

int TcpSocket::send(const void* data, int length) {
    ssize_t sentSize = ::send(commFd_, data, length, 0);
    if (sentSize == -1) {
        // 写入的连接关闭
        if (errno == EPIPE) {
            cerr << "send falied: client disconnected.";
            return -1;
        }
        // 其他写入错误
        else {
            cerr << "send failed: send syscall error.";
        }
    }
    return sentSize;
}
int TcpSocket::recv(void* buffer, int length) {
}
}  // namespace net