#include "socket/tcpSocketBase.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>
using std::string;

namespace net {
bool TcpSocketBase::fdIsValid() const {
    return sockFd_ != -1;
}

void TcpSocketBase::setError(const string& errorMessage) {
    errorMessage_ = errorMessage;
}

int TcpSocketBase::getSockFd() const {
    return sockFd_;
}

const std::string& TcpSocketBase::getError() const {
    return errorMessage_;
}

TcpSocketBase::~TcpSocketBase() {
    if (sockFd_ > 0) {
        ::close(sockFd_);
    }
}
}  // namespace net