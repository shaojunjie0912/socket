#include <arpa/inet.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

void ErrorHandling(char const *message) {
    std::cerr << message << std::endl;
    exit(1);
}

int main(int argc, char *argv[]) {
    char const *ip = "127.0.0.1";
    uint16_t const port = 9012;

    // 1. 创建客户端连接套接字
    int sock = socket(PF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);

    // 2. 连接服务器
    int ret = connect(sock, reinterpret_cast<sockaddr *>(&serv_addr),
                      sizeof(serv_addr));
    if (ret == -1) {
        ErrorHandling("connect error");
    }
    while (true) {
        char buf[1024]{};
        printf("输入要发送的消息(Q 退出): ");
        std::cin >> buf;
        if (!strcmp(buf, "Q\n") || !strcmp(buf, "q\n")) {
            break;
        }
        // 3. 向服务器发送数据
        send(sock, buf, strlen(buf), 0);
        // 4. 从服务器接收数据
        int cnt_msg_rcv = recv(sock, buf, sizeof(buf), 0);
        if (cnt_msg_rcv == 0) {
            close(sock);
            printf("服务器断开连接\n");
            return -1;
        } else {
            printf("收到服务器发送的消息: %s\n", buf);
        }
    }
    close(sock);
    return 0;
}
