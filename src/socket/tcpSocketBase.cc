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

TcpSocketBase::TcpSocketBase(TcpSocketBase&& other) noexcept : sockFd_(other.sockFd_) {
    other.sockFd_ = -1;
    errorMessage_ = std::move(other.errorMessage_);
}

TcpSocketBase& TcpSocketBase::operator=(TcpSocketBase&& other) noexcept {
    if (&other == this) {
        return *this;
    }
    if (fdIsValid()) {
        ::close(sockFd_);
    }
    sockFd_ = other.sockFd_;
    other.sockFd_ = -1;
    errorMessage_ = std::move(other.errorMessage_);
    return *this;
}

TcpSocketBase::~TcpSocketBase() {
    if (sockFd_ > 0) {
        ::close(sockFd_);
    }
}
}  // namespace net