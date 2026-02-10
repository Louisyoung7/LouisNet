#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "socket/InetAddress.h"
#include "utils/Buffer.h"
#include "socket/TcpConnection.h"

namespace reactor {
class EventLoop;
}

namespace net {
class Acceptor;
}  // namespace net

namespace server {
class TcpServer {
   public:
    // 连接智能指针类型定义
    using TcpConnectionPtr = std::shared_ptr<net::TcpConnection>;
    // 回调函数类型定义
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, utils::Buffer&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

    // 构造函数
    TcpServer(reactor::EventLoop* loop, const net::InetAddress& listenAddr);
    // 析构函数
    ~TcpServer();

    // 获取监听地址信息
    const net::InetAddress& listenAddr() const {
        return listenAddr_;
    }

    // 启动服务器
    void start();

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

   private:
    // 处理新连接
    void onNewConnection(int sockfd, const net::InetAddress& peerAddr);
    // 处理连接回调
    void onConnection(const TcpConnectionPtr& conn);
    // 处理关闭事件
    void onClose(const TcpConnectionPtr& conn);
    reactor::EventLoop* loop_;                       ///< 所属的EventLoop
    const net::InetAddress listenAddr_;              ///< 监听地址
    std::string name_;                               ///< 服务器名称
    std::unique_ptr<net::Acceptor> acceptor_;        ///< 接收器实例
    std::map<int, TcpConnectionPtr> connections_;  ///< 存储连接实例

    ConnectionCallback connectionCallback_;        ///< 连接回调
    MessageCallback messageCallback_;              ///< 消息接收回调
    WriteCompleteCallback writeCompleteCallback_;  ///< 写完成回调
};
}  // namespace server