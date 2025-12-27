#include <arpa/inet.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

void ErrorHandling(char const *message) {
    std::cerr << message << std::endl;
    exit(1);
}

int main(int argc, char *argv[]) {
    uint16_t const port = 9012;

    // 1. 创建服务器监听套接字 socket
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        ErrorHandling("socket() error");
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;    // 地址族
    serv_addr.sin_port = htons(port);  // 主机字节序->网络字节序
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // 2. 绑定监听套接字 bind
    int ret = bind(serv_sock, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr));
    if (ret == -1) {
        ErrorHandling("bind() error");
    }

    // 3. 开始监听 listen
    ret = listen(serv_sock, 5);
    if (ret == -1) {
        ErrorHandling("listen() error");
    }
    sockaddr_in clnt_addr;
    uint32_t clnt_addr_size{sizeof(clnt_addr)};

    while (true) {
        // 4. 接受客户端请求 accept
        // 5. 返回通信套接字
        int clnt_sock =
            accept(serv_sock, reinterpret_cast<sockaddr *>(&clnt_addr), &clnt_addr_size);
        if (clnt_sock == -1) {
            ErrorHandling("accept() error");
        }
        // 连接成功后, 打印客户端的ip和端口(网络字节序 -> 主机字节序)
        char ip[50] = {0};
        printf("检测到有客户端连接: %s, port: %d\n",
               inet_ntop(AF_INET, &clnt_addr.sin_addr.s_addr, ip, sizeof(ip)),
               ntohs(clnt_addr.sin_port));
        std::vector<std::byte> buf(1024);
        while (true) {
            buf.clear();
            // 6. 从通信套接字接收客户端发送的数据 recv 阻塞
            int len = recv(clnt_sock, reinterpret_cast<void *>(buf.data()), sizeof(buf), 0);
            if (len > 0) {
                // printf("接收到客户端发送的消息: %s\n", buf);
                printf("客户端消息: %d 字节\n", len);
                // 7. 向通信套接字发送数据 send
                send(clnt_sock, reinterpret_cast<void const *>(buf.data()), len, 0);
            } else if (len == 0) {
                printf("客户端断开连接...\n");
                break;
            } else {
                ErrorHandling("read error");
            }
        }
        close(clnt_sock);  // 关闭通信套接字
    }
    close(serv_sock);  // 关闭监听套接字
    return 0;
}
