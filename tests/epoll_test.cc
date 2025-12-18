#include "epoll.hpp"

#include <arpa/inet.h>
#include <socket/server.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "utils.hpp"
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::vector;

int main() {
    // 忽略SIGPIPE信号，防止因为向已经关闭的连接写数据导致程序崩溃
    utils::sigIgnore();

    // 创建TcpServer实例
    net::TcpServer server;

    server.start();

    cout << "Server listening..." << endl;

    int lfd = server.getListenFd();

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
            int new_fd = events[i].data.fd;

            // 情况1，监听套接字就绪，说明有客户端的连接请求
            if (new_fd == lfd) {
                while (true) {
                    // 这里不关心客户端的地址信息，单纯想建立连接，因此addr和addr_len都传nullptr
                    net::Connector conn = server.accept();
                    int cfd = conn.getSockFd();
                    int errNo = conn.getErrorNo();
                    if (cfd == -1) {
                        // 将所有的cfd都注册到了epoll
                        if (errNo == EAGAIN || errNo == EWOULDBLOCK) {
                            cout << "Accept complete" << endl;
                            break;
                        } else {
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
                    net::Connector conn(new_fd);
                    ssize_t n = conn.recv(buf, sizeof(buf));
                    int errNo = conn.getErrorNo();
                    if (n > 0) {
                        // 读取到n字节数据
                        buffer.append(buf, n);
                        cout << "Successfully read " << n << " bytes from fd=" << new_fd << endl;
                    } else if (n == 0) {
                        // 客户端断开连接
                        cout << "Client fd=" << new_fd << " disconnected." << endl;
                        epoll.delFd(new_fd);
                        close(new_fd);
                        break;
                    } else {
                        if (errNo == EAGAIN || errNo == EWOULDBLOCK) {
                            // 数据读完了
                            // 数据读完之后，内核会自动将fd转换为非就绪态
                            cout << "Read complete from fd=" << new_fd << ", echoing..." << endl;
                            ssize_t n_written = conn.send(buffer.c_str(), buffer.size());
                            int errNo = conn.getErrorNo();

                            if (n_written == -1) {
                                // write失败
                                if (errNo == EPIPE) {
                                    // 写入的连接已关闭
                                    cerr << "Write to fd=" << to_string(new_fd)
                                         << " failed: Broken pipe (EPIPE). Client disconnected." << endl;
                                } else {
                                    // 其他写入错误
                                    cerr << "Write error on fd=" << to_string(new_fd) << endl;
                                }
                                // 无论哪种错误，都要清理资源
                                epoll.delFd(new_fd);
                                close(new_fd);
                                break;
                            } else {
                                // write成功
                                cout << "Successfully wrote " << n_written << " bytes to fd=" << new_fd << endl;
                                break;
                            }
                        } else {
                            // 读取出错
                            cerr << "read error" << endl;
                            epoll.delFd(new_fd);
                            close(new_fd);
                            break;
                        }
                    }
                }  // 结束处理客户端数据
            }
        }  // 结束遍历返回的事件
    }  // 结束主事件循环

    return 0;
}