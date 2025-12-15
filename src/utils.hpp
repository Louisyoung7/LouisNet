#pragma once

#include <fcntl.h>       // for fcntl
#include <signal.h>      // for signal
#include <sys/socket.h>  // for setsockopt

#include <iostream>

namespace utils {
/**
 * @brief 将fd设置为非阻塞模式
 * @param fd 文件描述符
 * @return true 设置成功
 * @return false 设置失败，并打印日志
 */
inline bool setNonBlocking(int fd) {
    int flag = ::fcntl(fd, F_GETFL);
    if (flag == -1) {
        std::cerr << "serNonBlocking failed" << std::endl;
        return false;
    }

    if (::fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        std::cerr << "serNonBlocking failed" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief 设置地址复用
 * @param socket 套接字
 * @return true 设置成功
 * @return false 设置失败，并打印日志
 */
inline bool setReuseAddr(int socket) {
    int opt = 1;
    if (::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "set SO_REUSEADDR failed" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief 忽略SIGPIPE信号
 * 防止因为向已经关闭的连接写数据导致程序崩溃
 * @return true 设置成功
 * @return false 设置失败，并打印日志
 */
inline bool sigIgnore() {
    if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        std::cerr << "sigIgnore failed" << std::endl;
        return false;
    }

    return true;
}
}  // namespace utils