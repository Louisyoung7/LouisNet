#pragma once

#include <functional>
#include <memory>


class EventLoop;
class Socket;
class Channel;

namespace net {
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
   public:
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, std::string&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

   private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

    void setState(StateE state) {
        state_ = state;
    }

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    EventLoop* loop_;
    std::string name_;
    StateE state_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    Channel localAddr_;
    Channel peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
};
}  // namespace net
