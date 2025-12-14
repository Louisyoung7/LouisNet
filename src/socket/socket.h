#pragma once

#include <string>
using std::string;

namespace net {
class TcpSocket {
   protected:
    int sockFd_;
    int lastError;  ///< 存储最新的errno

   protected:
    /*
     * @brief 对bind的封装，增加了错误日志
     * @param ip 要指定的IP地址
     * @param port 要指定的端口号
     * return true bind成功
     * return false bind失败，打印日志
     */
    bool bind(const string& ip, int port);

    /*
     * @brief 对listen的封装
     * @return true listen成功
     * @return false listen失败，打印日志
     */
    bool listen();
    void accept();
    void connect();

    /*
     * @brief 获取最新的errno
     * return 最新的errno
     */
    int getErrno() const;

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