// echo_client.cc

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

int main() {
    // 1.创建通信套接字
    int cfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2.尝试与服务器端建立连接
    // 服务器端的地址信息
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);
    serverAddr.sin_port = htons(10000);
    if (connect(cfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect failed...");
    }

    // 3.通信开始
    char* msg = "Hello,Server!";
    char buf[1024];

    while (1) {
        if (send(cfd, msg, strlen(msg), 0) == -1) {
            perror("send failed...");
        } else {
            printf("send success: ");
            memset(buf, 0, sizeof(buf));
            recv(cfd, buf, sizeof(buf), 0);
            printf("%s\n", buf);
        }
    }

    // 4.回收资源
    close(cfd);
}