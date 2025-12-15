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
        cerr << "start failed: " << ::strerror(lastError) << endl;
        return false;
    }

    // 绑定端口
    if (!bind(ip, port)) {
        cerr << "start failed: " << ::strerror(lastError) << endl;
        ::close(sockFd_);
        return false;
    }

    // 设置监听
    if (!listen()) {
        cerr << "start failed: " << ::strerror(lastError) << endl;
        ::close(sockFd_);
        return false;
    }

    return true;
}
}  // namespace net
