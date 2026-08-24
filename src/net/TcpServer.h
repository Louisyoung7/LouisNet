#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "base/Buffer.h"
#include "base/noncopyable.h"
#include "net/InetAddress.h"
#include "net/TcpConnection.h"

namespace net {
namespace reactor {
class EventLoop;
class EventLoopThreadPool;
}  // namespace reactor

class Acceptor;

class TcpServer : public base::noncopyable {
   private:
    // 回调函数类型定义
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, base::Buffer&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

   public:
    TcpServer(reactor::EventLoop* loop, const net::InetAddress& listenAddr, const std::string& name = "TcpServer",
              bool reusePort = false);
    ~TcpServer();

    void start();

    void setThreadNum(int numThreads);

    const net::InetAddress& listenAddr() const { return listenAddr_; }

    // 设置回调函数
    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

   private:
    // 处理新连接
    void onNewConnection(int sockfd, const net::InetAddress& peerAddr);
    // 处理连接回调
    void onConnection(const TcpConnectionPtr& conn);
    // 移除连接
    void removeConnection(const TcpConnectionPtr& conn);
    // 在IO线程移除连接
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    reactor::EventLoop* mainLoop_;                              ///< acceptor 所属的EventLoop
    const InetAddress listenAddr_;                              ///< 监听地址
    const std::string name_;                                    ///< 服务器名称
    std::unique_ptr<Acceptor> acceptor_;                        ///< 接收器
    std::unique_ptr<reactor::EventLoopThreadPool> threadPool_;  ///< IO线程池
    std::map<int, TcpConnectionPtr> connections_;               ///< 连接实例映射表

    ConnectionCallback connectionCallback_;        ///< 连接回调
    MessageCallback messageCallback_;              ///< 消息接收回调
    WriteCompleteCallback writeCompleteCallback_;  ///< 写完成回调
};
}  // namespace net