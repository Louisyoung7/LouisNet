#include <arpa/inet.h>
#include <socket/server.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
using std::cerr;
using std::endl;
using std::string;

namespace net {
bool TcpServer::start(const string& ip, int port) {
    // 创建套接字
    sockFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ == -1) {
        return false;
    }

    // 绑定端口
    if (!bind(ip, port)) {
        ::close(sockFd_);
        return false;
    }

    // 设置监听
    if (!listen()) {
        ::close(sockFd_);
        return false;
    }

    return true;
}
}  // namespace net
