#pragma once

namespace net {
class TcpSocket {
   protected:
    int sockFd_;

   protected:
    bool bind();
    bool listen();
    void accept();
    void connect();

   public:
    TcpSocket() : sockFd_(-1){};

    /*
     * @brief 开启服务器
     * 底层会调用socket，bind，listen函数
     * @return true 开启成功
     * @return false 开启失败，打印日志
     */
    bool start();
    virtual ~TcpSocket();
    virtual void send();
    virtual void recv();
};
}  // namespace net