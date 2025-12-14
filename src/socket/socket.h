#pragma once

#include <string>
using std::string;

namespace net {
class TcpSocket {
   protected:
    int sockFd_;

   protected:
    bool bind(const string& ip, int port);
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
    bool start(const string& ip, int port);
    virtual ~TcpSocket();
    virtual void send();
    virtual void recv();
};
}  // namespace net