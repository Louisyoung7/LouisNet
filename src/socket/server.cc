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
bool TcpServer::start(const string& ip, int port, int backlog) {
    if (!listener.bindAndListen(ip, port,backlog)) {
        cerr << listener.getError() << endl;
        return false;
    }

    return true;
}

int TcpServer::getListenFd() const {
    return listener.getSockFd();
}
}  // namespace net
