#include "socket/socket.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <cerrno>
using std::cerr;
using std::endl;

namespace net {

bool TcpSocket::start() {
    // 创建套接字
    sockFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ == -1) {
        cerr << "socket failed: " << ::strerror(errno) << endl;
        return false;
    }

    // 绑定端口
    if (!bind()) {
        cerr << "bind failed: " << ::strerror(errno) << endl;
        ::close(sockFd_);
        return false;
    }

    // 设置监听
    if (!listen()) {
        cerr << "listen failed: " << ::strerror(errno) << endl;
        ::close(sockFd_);
        return false;
    }

    return true;
}

bool TcpSocket::bind() {
    // TODO
}
bool TcpSocket::listen() {
    // TODO
}
void TcpSocket::accept() {
    // TODO
}
void TcpSocket::connect() {
    // TODO
}
}  // namespace net