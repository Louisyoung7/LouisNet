#pragma once

#include <functional>

class EventLoop;
class Socket;
class Channel;


namespace net {
class Acceptor {
   public:
    using NewConnectionCallback = std::function<void(Socket* socket)>;

    // 构造析构
    Acceptor(EventLoop* loop, Socket* socketSocket);
    ~Acceptor();

    // 设置新连接回调
    void setNewConnectionCallback(NewConnectionCallback callback) {
        newConnectionCallback_ = std::move(callback);
    }

    // 是否正在监听
    bool listening() const {
        return listening_;
    }

   private:
    // 处理新连接
    void handleRead();

    EventLoop* loop_;
    Socket* acceptSocket_;
    Channel* acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};
}  // namespace net
