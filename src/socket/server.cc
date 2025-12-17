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
    if (!listener.bindAndListen(ip, port)) {
        cerr << listener.getError() << endl;
        return false;
    }

    return true;
}
}  // namespace net
