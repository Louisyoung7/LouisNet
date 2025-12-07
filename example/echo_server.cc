// echo_server.cc

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>  // for epoll APIs

#include <cstdio>
#include <cstring>
#include <iostream>
using std::cout;
using std::endl;

constexpr int PORT = 10000;
constexpr int MAX_EVENTS = 1024;    // epoll_wait最多能返回的事件数

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

                cout << "New client connected: fd=" << cfd << endl;

                // 将cfd也注册到epoll中，并关注可读事件
                ev.events = EPOLLIN;
                ev.data.fd = cfd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
                    perror("epoll_ctl: cfd add failed");
                    close(cfd);
                }
            } else {
                // 情况2，通信套接字就绪，说明收到了信息
                char buf[1024];
                int n = read(fd, buf, sizeof(buf) - 1); // 如果buf占满了，留一个字节来装'\0'

                // 客户端断开连接或者出错
                if (n <= 0) {
                    if (n == 0) {
                        cout << "Client fd=" << fd << " disconnected" << endl;
                    } else {
                        perror("read error");
                    }
                    // 不管是断开连接或者出错，都要关闭fd并从epoll中删除
                    close(fd);
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);   // 删除和添加不同，不需要知道绑定了什么事件，因此event传nullptr
                } else {
                    buf[n] = '\0';
                    cout << "Recv from fd=" << fd << ": " << buf << endl;
                    write(fd,buf,n);
                }
            }
        }
    }

    close(lfd);
    close(epfd);

    return 0;
}