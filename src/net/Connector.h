#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "base/noncopyable.h"
#include "net/InetAddress.h"
#include "net/reactor/Channel.h"

namespace net {
namespace reactor {
class EventLoop;
class Channel;
}  // namespace reactor

class Connector : public base::noncopyable, public std::enable_shared_from_this<Connector> {
   public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    Connector(reactor::EventLoop* loop, const InetAddress& serverAddr);
    ~Connector() = default;

    // 启动连接
    void start();
    // 停止连接
    void stop();
    // 重启连接，必须在事件循环中调用
    void restart();

    void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = std::move(cb); }

   private:
    // 连接状态
    enum class State {
        kDisconnected,
        kConnecting,
        kConnected,
    };

    reactor::EventLoop* loop_;
    InetAddress serverAddr_;                     // 对端地址
    std::unique_ptr<reactor::Channel> channel_;  // 工具Channel，用于监听连接事件
    bool connected_;                             // 是否已连接
    std::atomic<State> state_;                   // 连接状态
    NewConnectionCallback newConnectionCallback_;

    // Loop线程启动连接
    void startInLoop();
    // Loop线程停止连接
    void stopInLoop();

    // 创建socket并尝试连接对端
    void connect();
    // 连接过程中的操作
    void connecting(int sockfd);

    // 获取已连接的socket fd，并从EventLoop中移除Channel
    int getSocket();

    // 重置Channel
    void resetChannel();

    // 处理写事件
    // 取出已连接的socket fd，调用新连接回调函数
    void handleWrite();
    // 处理错误事件
    // 取出已连接的socket fd，关闭Channel和socket
    void handleError();
};
}  // namespace net
