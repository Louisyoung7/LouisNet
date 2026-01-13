#include "server/server.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
using std::cerr;
using std::endl;
using std::string;

namespace net {
bool TcpServer::start(const string& ip, int port, int backlog) {
    if (!listener_.create()) {
        cerr << listener_.getError() << endl;
        return false;
    }

    if (!listener_.bindAndListen(ip, port, backlog)) {
        cerr << listener_.getError() << endl;
        return false;
    }

    return true;
}

int TcpServer::getListenFd() const {
    return listener_.getSockFd();
}

int TcpServer::accept() {
    int cfd = listener_.accept();
    if (cfd == -1) {
        cerr << listener_.getError() << endl;
    }
    return cfd;
}
}  // namespace net
