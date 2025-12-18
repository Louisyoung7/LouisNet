#pragma once

#include <string>

namespace net {
class TcpSocketBase {
   protected:
    int sockFd_;
    std::string errorMessage_;  ///< 存储错误信息
   protected:
    /**
     * @brief 判断底层套接字是否没有问题
     */
    bool fdIsValid() const;

    /**
     * @brief 设置错误信息
     */
    void setError(const std::string& errorMessage);

   public:
    /**
     * @brief 构造函数，初始化成员变量
     */
    TcpSocketBase() : sockFd_(-1), errorMessage_(""){};

    /**
     * @brief 构造函数，通过已知套接字初始化
     */
    TcpSocketBase(int sockFd) : sockFd_(sockFd), errorMessage_(""){};

    /**
     * @brief 虚析构函数
     */
    virtual ~TcpSocketBase();

    /**
     * @brief 获取套接字
     */
    int getSockFd() const;

    /**
     * @brief 获取最新的错误信息
     */
    const std::string& getError() const;
};
}  // namespace net