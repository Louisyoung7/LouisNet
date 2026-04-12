#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "InetAddress.h"
#include "Socket.h"
#include "base/Buffer.h"
#include "base/noncopyable.h"
#include "reactor/EventLoop.h"

namespace net {

namespace reactor {
class Channel;
}  // namespace reactor

class TcpConnection : public std::enable_shared_from_this<TcpConnection>, public base::noncopyable {
   public:
    // 智能指针类型定义
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

   private:
    // 回调函数类型定义
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, base::Buffer&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

    // 连接状态枚举
    enum class StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

    reactor::EventLoop* loop_;        ///< 所属的EventLoop
    std::unique_ptr<Socket> socket_;  ///< 底层通信套接字
    std::string name_;                ///< 连接名称
    StateE state_;                    ///< 连接状态
    int error_;                       ///< 错误状态

    std::unique_ptr<reactor::Channel> channel_;  ///< 连接的Channel
    const InetAddress localAddr_;                ///< 本端地址
    const InetAddress peerAddr_;                 ///< 对端地址

    // 回调函数
    ConnectionCallback connectionCallback_;        ///< 连接建立/销毁回调
    MessageCallback messageCallback_;              ///< 消息接收回调
    WriteCompleteCallback writeCompleteCallback_;  ///< 写完成回调
    CloseCallback closeCallback_;                  ///< 连接关闭回调

    base::Buffer outputBuffer_;  ///< 发送缓冲区
    base::Buffer inputBuffer_;   ///< 接收缓冲区

   public:
    // 构造析构
    TcpConnection(reactor::EventLoop* loop, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    // 获取相关信息
    reactor::EventLoop* getLoop() const {
        return loop_;
    }
    int fd() const {
        return socket_->fd();
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

    // 查询是否已连接
    bool connected() const {
        return state_ == StateE::kConnected;
    }
    // 查询是否断开连接
    bool disconnected() const {
        return state_ == StateE::kDisconnected;
    }

    // 发送数据
    void send(const std::string& message);
    // 发送数据
    void send(const void* data, size_t len);

    // 关闭连接
    void shutdown();

    // 获取错误状态
    int getError() const {
        return error_;
    }

    // 设置回调函数
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

    // 连接建立
    void connectionEstablished();

    // 连接销毁
    void connectionDestroyed();

   private:
    // 事件处理函数
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    // 在IO线程中发送数据
    void sendInLoop(const void* data, size_t len);
    // 在IO线程中关闭连接
    void shutdownInLoop();

    void setState(StateE s) {
        state_ = s;
    }
};
}  // namespace net
