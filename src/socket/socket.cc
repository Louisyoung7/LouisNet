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

bool TcpSocket::start(const string& ip, int port) {
    // 创建套接字
    sockFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ == -1) {
        cerr << "socket failed: " << ::strerror(errno) << endl;
        return false;
    }

    // 绑定端口
    if (!bind(ip,port)) {
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

bool TcpSocket::bind(const string& ip, int port) {
    if (sockFd_ < 0) {
        cerr << "Invalid socket." << endl;
        return false;
    }
    
    // 设置服务器地址信息
    struct sockaddr_in sockAddr{};
    sockAddr.sin_family = AF_INET;  ///< 地址族

    // 指定ip，错误判断
    int ptonRet = ::inet_pton(AF_INET, ip.c_str(), &sockAddr.sin_addr.s_addr);
    if (ptonRet != 1) {
        if (ptonRet == 0) {
            cerr << "Invalid IP address." << endl;
        }
        else if (ptonRet == -1) {
            cerr << "inet_pton call failed" << endl;
        }
        return false;
    }

    sockAddr.sin_port = htons(port);  ///< 端口，注意字节序转换
    
    if (::bind(sockFd_, reinterpret_cast<struct sockaddr*>(&sockAddr), sizeof(sockAddr)) == -1) {
        return false;
    }

    return true;
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