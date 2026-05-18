#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "net/TcpConnection.h"

namespace base {
class Buffer;
}

namespace net {
namespace reactor {
class EventLoop;
}

class Connector;
class TcpConnection;
class InetAddress;

class TcpClient {
   private:
    using ConnectorPtr = std::shared_ptr<Connector>;

    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, base::Buffer&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

   public:
    TcpClient(reactor::EventLoop* loop, const InetAddress& serverAddr, const std::string& name = "TcpClient");
    ~TcpClient();

    // 主动发起连接
    void connect();
    // 主动断开已建立的连接
    void disconnect();
    // 停止正在连接的连接器
    void stop();

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

    reactor::EventLoop* getLoop() const { return loop_; }

   private:
    // 新连接
    void onNewConnection(int sockfd);
    // 移除连接
    void onRemoveConnection(const TcpConnectionPtr& conn);

    reactor::EventLoop* loop_;
    ConnectorPtr connector_;    // 连接器实例
    const std::string name_;    // 客户端名称
    mutable std::mutex mutex_;  // 互斥锁

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    TcpConnectionPtr connection_;  // 连接实例
    int nextConnId_;               // 下一个连接ID，用于唯一标识每个连接
};

}  // namespace net
