#pragma once

#include <string>

namespace net {
class TcpSocket {
   protected:
    int sockFd_;
    int lastError;  ///< 存储最新的errno

   protected:
    /**
     * @brief 对bind的封装，增加了错误日志
     * @param ip 要指定的IP地址
     * @param port 要指定的端口号
     * @return true bind成功
     * @return false bind失败，打印日志
     */
    bool bind(const std::string& ip, int port);

    /**
     * @brief 对listen的封装
     * @return true listen成功
     * @return false listen失败，打印日志
     */
    bool listen();

    /**
     * @brief 对accept的封装
     * @return 返回通信套接字，如果出错返回-1
     */
    int accept();


    bool connect(const std::string& ip, int port);

    /**
     * @brief 获取最新的errno
     * @return 最新的errno
     */
    int getErrno() const;

    /**
     * @brief 判断底层套接字是否没有问题
     * @return true 没问题
     * @return false 有问题，打印日志
     */
    bool sockFdIsValid() const;

   public:
    TcpSocket() : sockFd_(-1){};

    /**
     * @brief 开启服务器
     * 底层会调用socket，bind，listen函数
     * @return true 开启成功
     * @return false 开启失败，打印日志
     */
    bool start(const std::string& ip, int port);
    virtual ~TcpSocket();
    virtual void send();
    virtual void recv();
};
}  // namespace net