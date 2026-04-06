#include "TcpConnection.h"

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <exception>
#include <memory>
#include <sstream>

#include "base/LouisLog.h"
#include "reactor/Channel.h"

using namespace net;
using namespace net::reactor;

TcpConnection::TcpConnection(EventLoop* loop, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(loop),
      socket_(std::make_unique<Socket>(sockfd)),
      state_(StateE::kConnecting),
      error_(0),
      channel_(std::make_unique<Channel>(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr) {
    // 生成连接名称
    std::ostringstream oss;
    oss << "conn-" << socket_->fd() << "-" << peerAddr.toIpPort();
    name_ = oss.str();

    // 默认设置TCP_NODELAY选项，减少延迟
    socket_->setTcpNoDelay(true);
    // 设置保持连接选项，防止连接超时
    socket_->setKeepAlive(true);

    // 设置Channel的事件回调，绑定TcpConnection相应的成员函数
    channel_->setReadCallback([this]() { handleRead(); });
    channel_->setWriteCallback([this]() { handleWrite(); });
    channel_->setCloseCallback([this]() { handleClose(); });
    channel_->setErrorCallback([this]() { handleError(); });

    DEBUG_F("[TcpConnection] TcpConnection() created connection %s.\n\n", name_.c_str());
}
TcpConnection::~TcpConnection() {
    // 析构函数调用时，确保连接已经断开
    assert(state_ == StateE::kDisconnected);

    DEBUG_F("[TcpConnection] ~TcpConnecion() destroying connection %s.\n\n", name_.c_str());
}

// 连接建立，更新连接状态为kConnected，使能Channel的读事件，调用连接建立回调
void TcpConnection::connectionEstablished() {
    assert(state_ == StateE::kConnecting);
    // 更新连接状态
    setState(StateE::kConnected);
    // 使能Channel的读事件
    channel_->enableRead();
    // 调用连接建立回调
    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }

    DEBUG_F("[TcpConnection] connectionEstablished() connection %s established.\n\n", name_.c_str());
}

// 连接关闭，更新连接状态为kDisconnected，关闭Channel的所有事件，并将Channel从所属的EventLoop中移除
void TcpConnection::connectionDestroyed() {
    // 无论之前是何种状态，现在一律更新为kDisconnected
    if (state_ != StateE::kDisconnected) {
        setState(StateE::kDisconnected);
    }

    try {
        // 关闭Channel所有事件
        channel_->disableAll();
        // 从所属的EventLoop中移除
        channel_->remove();

        DEBUG_F("[TcpConnection] connectionDestroyed() connection %s destroyed.\n\n", name_.c_str());
    } catch (const std::exception& e) {
        ERROR_F("[TcpConnection] connectionDestroyed() error: %s.\n\n", e.what());
    }

    // TcpConnection对象的真正销毁由智能指针管理，当引用计数归零，自动调用析构函数
}

// 从sockfd读取数据到接收缓冲区，如果有数据则调用消息接收回调，无数据则关闭连接
void TcpConnection::handleRead() {
    // 用于获取和保存errno，方便判断错误
    int savedError = 0;

    // 从sockfd读取数据到接收缓冲区
    ssize_t n = inputBuffer_.readFd(socket_->fd(), &savedError);

    if (n > 0) {
        // 接收到数据，调用消息接收回调
        if (messageCallback_) {
            messageCallback_(shared_from_this(), inputBuffer_);
        }
    } else if (n == 0) {
        // 连接关闭，处理关闭事件
        handleClose();
    }

    else {
        // 更新errno
        errno = savedError;
        ERROR_F("[TcpConnection] handleRead() error: %s.\n\n", strerror(errno));
        // 处理错误事件
        handleError();
    }
}
// 将发送缓冲区的数据写入到sockfd，如果写完，禁用写事件，调用写完成回调
void TcpConnection::handleWrite() {
    // 检查是否关注了写事件
    if (channel_->isWriting()) {
        // 用于获取和保存errno，方便判断错误
        int savedError = 0;

        // 用发送缓冲区向sockfd写入数据
        ssize_t n = outputBuffer_.writeFd(socket_->fd(), &savedError);

        if (n > 0) {
            // 数据写出完了
            if (outputBuffer_.readableBytes() == 0) {
                // 禁用写事件，节省系统资源
                channel_->disableWrite();

                // 调用写完成回调
                if (writeCompleteCallback_) {
                    writeCompleteCallback_(shared_from_this());
                }

                // 如果连接正在关闭，调用shutdown关闭写端
                if (state_ == StateE::kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            // 向sockfd写入错误
            errno = savedError;
            ERROR_F("[TcpConnection] handleWrite() error: %s.\n\n", strerror(errno));
            // 处理错误事件
            handleError();
        }
    }
}
// 处理连接关闭，更新连接状态，调用连接销毁回调和连接关闭回调
void TcpConnection::handleClose() {
    assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);

    DEBUG_F("[TcpConnection] handleClose() connection %s closing.\n\n", name_.c_str());
    setState(StateE::kDisconnected);

    // 创建一个shared_ptr来延长对象生命周期
    //? 这里为什么需要创建一个shared_ptr？
    // 因为在handleClose()中，会调用closeCallback_，而在closeCallback_内部可能有逻辑会从持有该对象的容器（如服务器的连接列表）中移除这个连接
    // 如果这是最后一个 shared_ptr 引用，那么 TcpConnection 的析构函数会在回调函数执行期间就被调用
    // 而这时回调函数仍在执行，可能会访问到已经被销毁的对象
    // 为了确保回调函数执行时对象仍然存在，我们创建一个shared_ptr来延长对象生命周期
    // 只有当 handleClose() 完全执行完毕， guardThis 局部变量被销毁后，对象才可能被析构（前提是没有其他引用）
    TcpConnectionPtr guardThis(shared_from_this());

    // 调用连接销毁回调
    if (connectionCallback_) {
        connectionCallback_(guardThis);
    }

    // 调用连接关闭回调
    if (closeCallback_) {
        closeCallback_(guardThis);
    }

    // 所有回调执行完，这时对象的生命周期才结束，自动销毁（前提是没有其他引用）
}
// 获取并打印sockfd的错误信息，然后调用handleClose()关闭连接
void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof(optval);

    // 获取sockfd的出错时的errno
    // 这里出现的错误相对于handleError()是异步的，不能直接用errno
    // 而是通过getsockopt获取SO_ERROR选项来获取最新的错误状态
    if (::getsockopt(socket_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        optval = errno;
    }

    // 保存错误状态
    error_ = optval;
    errno = optval;
    ERROR_F("[TcpConnection] handleError() connection %s error: %s.\n\n", name_.c_str(), strerror(errno));

    // 处理关闭事件
    handleClose();
}

// 发送string数据，底层直接调用write系统调用
void TcpConnection::send(const std::string& message) {
    send(message.data(), message.size());
}
// 发送原始数据，底层直接调用write系统调用
void TcpConnection::send(const void* data, size_t len) {
    // 确保当前连接状态是已连接
    if (state_ == StateE::kConnected) {
        sendInLoop(data, len);
    }
}

//? 输出缓冲区中的数据发送和直接通过write系统调用快速发送数据的时序逻辑是什么
// 1.条件检查 ： !channel_->isWriting() && outputBuffer_.readableBytes() == 0
// (1)确保没有正在进行的写事件
// (2)确保输出缓冲区中没有任何待发送的数据
// 2.顺序保证 ：只有在输出缓冲区为空的情况下，当前要发送的数据才是最紧急需要发送的数据
// 3.原子性处理 ：如果直接 write 不能完全发送，剩余部分会被添加到输出缓冲区的末尾，等待后续发送
// 4.顺序一致性 ：由于 TCP 协议本身的有序性加上上述逻辑保证，数据的发送顺序与应用程序调用 send 的顺序一致

// 在IO线程中发送数据，底层直接调用write系统调用
void TcpConnection::sendInLoop(const void* data, size_t len) {
    ssize_t nwrote = 0;      ///< 记录已经写入多少数据到sockfd
    size_t remaining = len;  ///< 记录剩余未写入有多少数据
    int savedError = 0;      ///< 用于获取和保存errno，方便判断错误

    // 连接已关闭，放弃写入
    if (state_ == StateE::kDisconnected) {
        DEBUG_F("[TcpConnection] sendInLoop() connection %s is disconnected, give up writing.\n\n", name_.c_str());
        return;
    }

    // 当输出缓冲区为空（没有任何待发送的数据），且没有正在写事件，优先尝试立即写入
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(socket_->fd(), data, len);

        if (nwrote >= 0) {
            // 写入nwrote字节，更新剩余未写入字节数
            remaining = len - nwrote;
            // 如果全部写入，调用写完成回调
            if (remaining == 0 && writeCompleteCallback_) {
                writeCompleteCallback_(shared_from_this());
            }
        } else {
            // 写入错误，重置nwrote
            nwrote = 0;
            if (errno != EINTR) {
                ERROR_F("[TcpConnection] sendInLoop() connection %s error: %s.\n\n", name_.c_str(), strerror(errno));
                if (errno == EPIPE || errno == ECONNRESET) {
                    // 更新savedError
                    savedError = errno;
                    // 处理关闭事件
                    handleClose();
                }
            }
            // EINTR可以忽略
        }
    }

    // 部分写入，剩余的数据添加到发送缓冲区
    if (savedError == 0 && remaining > 0) {
        // 将剩余数据添加到发送缓冲区，等待后续发送
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);

        // 确保开启写事件
        if (!channel_->isWriting()) {
            channel_->enableWrite();
        }
    }
}

// 关闭连接
void TcpConnection::shutdown() {
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnecting);
        shutdownInLoop();
    }
}
// 在IO线程中关闭连接
void TcpConnection::shutdownInLoop() {
    // 只有在没有使能写事件（发送缓冲区可能为空时）才关闭写端
    if (!channel_->isWriting()) {
        ::shutdown(socket_->fd(), SHUT_WR);  // 半关闭，只关闭写端
    }
}

// 强制关闭连接
void TcpConnection::forceClose() {
    if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting) {
        setState(StateE::kDisconnecting);
        forceCloseInLoop();
    }
}
// 在IO线程中强制关闭连接
void TcpConnection::forceCloseInLoop() {
    if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting) {
        handleClose();
    }
}