#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "InetAddress.h"
#include "utils/Buffer.h"

namespace reactor {
class EventLoop;
class Channel;
}  // namespace reactor

namespace net {
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
   public:
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, utils::Buffer&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

    enum class StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

    TcpConnection(reactor::EventLoop* loop, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    reactor::EventLoop* getLoop() const {
        return loop_;
    }
    int fd() const {
        return sockfd_;
    }
    const InetAddress& localAddress() const {
        return localAddr_;
    }
    const InetAddress& peerAddress() const {
        return peerAddr_;
    }
    const std::string& name() const {
        return name_;
    }

    bool connected() const {
        return state_ == StateE::kConnected;
    }
    bool disconnected() const {
        return state_ == StateE::kDisconnected;
    }

    void send(const std::string& message);
    void send(const void* data, size_t len);

    void shutdown();
    void forceClose();

    void setTcpNoDelay(bool on);

    int getError() const {
        return error_;
    }

    void setConnectionCallback(ConnectionCallback cb) {
        connectionCallback_ = std::move(cb);
    }
    void setMessageCallback(MessageCallback cb) {
        messageCallback_ = std::move(cb);
    }
    void setWriteCompleteCallback(WriteCompleteCallback cb) {
        writeCompleteCallback_ = std::move(cb);
    }
    void setCloseCallback(CloseCallback cb) {
        closeCallback_ = std::move(cb);
    }

    void connectionEstablished();

    void connectionDestroyed();

   private:
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();
    void forceCloseInLoop();
    void setTcpNoDelayInLoop(bool on);

    void setState(StateE s) {
        state_ = s;
    }

    reactor::EventLoop* loop_;
    const int sockfd_;
    std::string name_;
    StateE state_;
    int error_;

    std::unique_ptr<reactor::Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;

    utils::Buffer outputBuffer_;
    utils::Buffer inputBuffer_;

};
}  // namespace net
