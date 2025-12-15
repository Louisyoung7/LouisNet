#pragma once

#include <string>

namespace net {
class TcpSocket {
   protected:
    int sockFd_;
    int commFd_;
    int lastError;  ///< 存储最新的errno

   protected:
    /**
     * @brief 对bind的封装
     * @param ip 要指定的IP地址
     * @param port 要指定的端口号
     * @return true bind成功
     * @return false bind失败，更新错误号
     */
    bool bind(const std::string& ip, int port);

    /**
     * @brief 对listen的封装
     * @return true listen成功
     * @return false listen失败，更新错误号
     */
    bool listen();

    /**
     * @brief 对accept的封装
     * @return 返回通信套接字，如果出错返回-1，更新错误号
     */
    int accept();

    /**
     * @brief 对connect的封装
     * @param ip 要指定的IP
     * @param port 要指定的端口号
     * @return true connect成功
     * @return false connect失败，更新错误号
     */
    bool connect(const std::string& ip, int port);

    /**
     * @brief 获取最新的errno
     * @return 最新的errno
     */
    int getErrno() const;

    /**
     * @brief 判断底层套接字是否没有问题
     * @return true 没问题
     * @return false 有问题，更新错误号
     */
    bool sockFdIsValid() const;

   public:
    TcpSocket() : sockFd_(-1), commFd_(-1), lastError(0){};

    /**
     * @brief 开启服务器
     * 底层会调用socket，bind，listen函数
     * @return true 开启成功
     * @return false 开启失败，更新错误号
     */
    bool start(const std::string& ip, int port);

    /**
     * @brief 虚析构函数，close套接字，释放资源
     */
    virtual ~TcpSocket();
    virtual int send(const void* data, int length);
    virtual int recv(void* buffer, int length);
};
}  // namespace net