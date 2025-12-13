#include "epoll.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <fcntl.h>      // for fcntl, O_NONBLOCK
#include <sys/epoll.h>  // for epoll APIs
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::vector;

constexpr int PORT = 8080;

int main() {
    // 忽略SIGPIPE信号，防止因为向已经关闭的连接写数据导致程序崩溃
    utils::sigIgnore();

    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket failed");
        return -1;
    }

    // 允许端口复用，避免出现端口正在被使用的报错
    utils::setReuseAddr(lfd);

    // 绑定IP端口
    // 设置服务器端的地址信息
    struct sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);  // 注意转换大小端
    // 绑定IP端口
    if (bind(lfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind failed");
        close(lfd);
        return -1;
    }

    // 设置监听
    // SOMAXCONN是一个系统默认值
    if (listen(lfd, SOMAXCONN) == -1) {
        perror("listen failed");
        close(lfd);
        return -1;
    }

    cout << "Server listening on port " << PORT << endl;

    // 设置监听套接字为非阻塞
    utils::setNonBlocking(lfd);

    // 创建epoll实例
    net::Epoll epoll;

    // 将监听套接字lfd注册到epoll中，ET模式
    epoll.addFd(lfd, EPOLLIN | EPOLLET);

    // 主事件循环
    while (true) {
        // 不断等待事件发生
        // 返回当前就绪事件的数量
        int nfds = epoll.wait();
        if (nfds == -1) {
            break;  // epoll_wait系统调用失败，一般此时出现了较严重错误，直接break；如果用continue，可能会一直失败
        }

        // 提取事件向量
        vector<struct epoll_event> events = epoll.getEvents();

        // 遍历所有就绪事件
        for (int i = 0; i < nfds; ++i) {
            int epoll_fd = events[i].data.fd;

            // 情况1，监听套接字就绪，说明有客户端的连接请求
            if (epoll_fd == lfd) {
                while (true) {
                    // 这里不关心客户端的地址信息，单纯想建立连接，因此addr和addr_len都传nullptr
                    int cfd = accept(lfd, nullptr, nullptr);
                    if (cfd == -1) {
                        // 将所有的cfd都注册到了epoll
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            cout << "Accept complete" << endl;
                            break;
                        } else {
                            // accept 出错
                            perror("accept failed");
                            break;
                        }
                    } else {
                        // 将获得的cfd设置为非阻塞
                        utils::setNonBlocking(cfd);

                        cout << "New client connected: fd=" << cfd << endl;

                        // 将cfd也注册到epoll中，ET模式
                        epoll.addFd(cfd, EPOLLIN | EPOLLET);
                    }
                }
            } else {
                cout << "-------------------------------" << endl;

                // 情况2，通信套接字就绪，说明收到了信息
                string buffer;  // 临时存储接收的数据
                char buf[1024];

                while (true) {
                    ssize_t n = read(epoll_fd, buf, sizeof(buf));
                    if (n > 0) {
                        // 读取到n字节数据
                        buffer.append(buf, n);
                        cout << "Successfully read " << n << " bytes from fd=" << epoll_fd << endl;
                    } else if (n == 0) {
                        // 客户端断开连接
                        cout << "Client fd=" << epoll_fd << " disconnected." << endl;
                        epoll.delFd(epoll_fd);
                        close(epoll_fd);
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 数据读完了
                            // 数据读完之后，内核会自动将fd转换为非就绪态
                            cout << "Read complete from fd=" << epoll_fd << ", echoing..." << endl;
                            ssize_t n_written = write(epoll_fd, buffer.c_str(), buffer.size());

                            if (n_written == -1) {
                                // write失败
                                if (errno == EPIPE) {
                                    // 写入的连接已关闭
                                    perror(("Write to fd=" + to_string(epoll_fd) +
                                            " failed: Broken pipe (EPIPE). Client disconnected.")
                                               .c_str());
                                } else {
                                    // 其他写入错误
                                    perror(("Write error on fd=" + to_string(epoll_fd)).c_str());
                                }
                                // 无论哪种错误，都要清理资源
                                epoll.delFd(epoll_fd);
                                close(epoll_fd);
                                break;
                            } else {
                                // write成功
                                cout << "Successfully wrote " << n_written << " bytes to fd=" << epoll_fd << endl;
                                break;
                            }
                        } else {
                            // 读取出错
                            perror("read error");
                            epoll.delFd(epoll_fd);
                            close(epoll_fd);
                            break;
                        }
                    }
                }  // 结束处理客户端数据
            }
        }  // 结束遍历返回的事件
    }  // 结束主事件循环

    close(lfd);

    return 0;
}