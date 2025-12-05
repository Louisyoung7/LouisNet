// echo_server.cc

#include <arpa/inet.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

int main() {
    // 1.创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket failed...");
    }

    // 2.绑定IP端口
    // 设置服务器端的地址信息
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(10000);  // 注意转换大小端
    // 绑定IP端口
    if (bind(lfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind failed...");
    }

    // 3.设置监听
    if (listen(lfd, 128) == -1) {
        perror("listen failed...");
    }

    perror("start server...");

    // 4.阻塞等待客户端的连接请求
    // 获取通信套接字
    int cfd = accept(lfd, NULL, NULL);
    if (cfd == -1) {
        perror("accept failed...");
    }

    // 缓冲区
    char buf[1024];

    // 5.通信开始
    while (1) {
        memset(buf, 0, sizeof(buf));

        if (recv(cfd, buf, sizeof(buf), 0) == -1) {
            perror("recv failed...");
        } else {
            printf("recv success: %s\n", buf);
            send(cfd, buf, strlen(buf), 0);
            sleep(1);
        }
    }

    // 6.回收资源
    close(lfd);
    close(cfd);

    return 0;
}