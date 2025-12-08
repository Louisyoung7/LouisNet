// echo_server.cc

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>  // for epoll APIs
#include <fcntl.h>  // for fcntl, O_NONBLOCK

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
using std::cout;
using std::endl;
using std::string;

constexpr int PORT = 10000;
constexpr int MAX_EVENTS = 1024;  // epoll_wait最多能返回的事件数

// 设置fd为非阻塞
void setNonBlocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_flag);
}

int main() {
    // 1.创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket failed");
        return -1;
    }

    // 允许端口复用，避免出现端口正在被使用的报错
    int opt = 1;  // 设置为1表示启用端口复用
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    // 2.绑定IP端口
    // 设置服务器端的地址信息
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);  // 注意转换大小端
    // 绑定IP端口
    if (bind(lfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind failed");
        close(lfd);
        return -1;
    }

    // 3.设置监听
    if (listen(lfd, 128) == -1) {
        perror("listen failed");
        close(lfd);
        return -1;
    }

    cout << "Server listening on port " << PORT << endl;

    // 4.创建epoll实例
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1 failed");
        close(lfd);
        return -1;
    }

    // 5.准备事件结构体
    struct epoll_event ev;  // 用于将需要监听的套接字注册到epoll中
    struct epoll_event events[MAX_EVENTS];  // 存储epoll_wait得到的就绪的套接字

    // 6.将监听套接字lfd注册到epoll中，关注EPOLLIN（可读）事件
    ev.events = EPOLLIN;    // 关注可读事件
    ev.data.fd = lfd;       // 将lfd绑定到这个事件
    // 将lfd以及要监听lfd的什么事件等信息注册到epoll中
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) == -1) {
        perror("epoll_ctl: lfd add failed");
        close(lfd);
        close(epfd);
        return -1;
    }

    // 主事件循环
    while (true) {
        // 7.不断等待事件发生
        // epoll_wait返回当前已经就绪的fd的个数，并且会填充events数组，timeout=-1表示一直阻塞等待
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait failed");
            break;  // epoll_wait系统调用失败，一般此时出现了较严重错误，直接break；如果用continue，可能会一直失败
        }

        // 8.遍历所有就绪事件
        for (int i = 0; i < nfds; ++i) {
            int fd = events->data.fd;

            // 情况1，监听套接字就绪，说明有客户端的连接请求
            if (fd == lfd) {
                // 这里不关心客户端的地址信息，单纯想建立连接，因此addr和addr_len都传nullptr
                int cfd = accept(lfd, nullptr, nullptr);
                if (cfd == -1) {
                    perror("accept failed");
                    continue;
                }

                // 将获得的cfd设置为非阻塞
                setNonBlocking(cfd);

                cout << "New client connected: fd=" << cfd << endl;

                // 将cfd也注册到epoll中，并设置为ET模式
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = cfd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
                    perror("epoll_ctl: cfd add failed");
                    close(cfd);
                }
            } else {
                // 情况2，通信套接字就绪，说明收到了信息
                string buffer;  // 临时存储接收的数据
                char buf[1024];

                while (true) {
                    ssize_t n = read(fd, buf, sizeof(buf));
                    if (n > 0) {
                        // 读取到n字节数据
                        buffer.append(buf,n);
                    }
                    else if (n == 0) {
                        // 客户端断开连接
                        cout << "Client fd=" << fd << " disconnected." << endl;
                        close(fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 数据读完了
                            // 数据读完之后，内核会自动将fd转换为非就绪态
                            cout << "Read complete from fd=" << fd << ", echoing..." << endl;
                            write(fd, buffer.c_str(), buffer.size());
                            break;
                        } else {
                            // 读取出错
                            perror("read error");
                            close(fd);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            break;
                        }
                    }
                }
            }
        }
    }

    close(lfd);
    close(epfd);

    return 0;
}