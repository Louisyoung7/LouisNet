#pragma once

#include <string>

namespace net {
class TcpSocket {
   protected:
    int sockFd_;
    int lastError;  ///< 存储最新的errno

   protected:
    /**
     * @brief 对bind的封装
     * @param ip 要指定的IP地址
     * @param port 要指定的端口号
     * @return true bind成功
     * @return false bind失败，更新错误号，打印日志
     */
    bool bind(const std::string& ip, int port);

    /**
     * @brief 对listen的封装
     * @return true listen成功
     * @return false listen失败，更新错误号，打印日志
     */
    bool listen();

    /**
     * @brief 对accept的封装
     * @return 返回通信套接字，如果出错返回-1，更新错误号，打印日志
     */
    int accept();

    /**
     * @brief 对connect的封装
     * @param ip 要指定的IP
     * @param port 要指定的端口号
     * @return true connect成功
     * @return false connect失败，更新错误号，打印日志
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
     * @return false 有问题，更新错误号，打印日志
     */
    bool sockFdIsValid() const;

   public:
    /**
     * @brief 普通构造函数，只初始化成员变量，多用于服务器端
     */
    TcpSocket() : sockFd_(-1), lastError(0){};

    /**
     * @brief 全能构造函数
     */
    TcpSocket(int sockFd) : sockFd_(sockFd), lastError(0){};

    /**
     * @brief 虚析构函数，close套接字，释放资源
     */
    virtual ~TcpSocket();
    virtual int send(const void* data, int length);
    virtual int recv(void* buffer, int length);
};
}  // namespace net