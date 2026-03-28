#pragma once

#include <functional>
#include <memory>

#include "Socket.h"
#include "base/noncopyable.h"

namespace net {

namespace reactor {
class EventLoop;
class Channel;
}  // namespace reactor

class InetAddress;

class Acceptor : public base::noncopyable {
   public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& peerAddr)>;

    // 构造析构
    Acceptor(reactor::EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    // 设置新连接回调
    void setNewConnectionCallback(NewConnectionCallback callback) {
        newConnectionCallback_ = std::move(callback);
    }

    // 开始监听
    void listen();

    // 是否正在监听
    bool listening() const {
        return listening_;
    }

   private:
    // 处理监听的socket上的读事件（新连接）
    void handleRead();

    reactor::EventLoop* loop_;                         ///< 所属的EventLoop
    bool listening_;                                   ///< 是否正在监听
    std::unique_ptr<Socket> acceptSocket_;             ///< 监听socket
    std::unique_ptr<reactor::Channel> acceptChannel_;  ///< 监听Channel
    NewConnectionCallback newConnectionCallback_;      ///< 新连接回调函数
};
}  // namespace net
