#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

int main() {
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9012);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(serv_sock, reinterpret_cast<sockaddr *>(&serv_addr),
                   sizeof(serv_addr));
    listen(serv_sock, 5);

    fd_set reads;
    FD_ZERO(&reads);           // reads 清零
    FD_SET(serv_sock, &reads); // 将监听套接字加入 reads 监听

    int max_fd = serv_sock; // 刚开始时, 最大 fd 就是监听套接字

    timeval out_time; // 设置超时时间

    while (true) {
        fd_set tmps = reads;     // 创建 reads 的副本, 因为 select 会修改 reads
        out_time.tv_sec = 5;     // NOTE: 必须在while内部
        out_time.tv_usec = 5000; // 因为 select 会修改 out_time 为剩余超时时间

        // 限时阻塞 select
        int fd_num = select(max_fd + 1, &tmps, nullptr, nullptr, &out_time);

        if (fd_num == 0) {
            continue; // 如果没有事件发生则跳转下一次
        }

        // 开始轮询
        for (int i = 0; i < max_fd + 1; ++i) {
            // 如果有事件发生
            if (FD_ISSET(i, &tmps)) { // NOTE: 这里是 tmps 而不是 reads
                // 1. 客户端连接事件
                if (i == serv_sock) {
                    sockaddr_in clnt_addr{};
                    socklen_t clnt_addr_len;
                    // NOTE: 此时 accept 一定不会阻塞
                    int clnt_sock = accept(
                        serv_sock, reinterpret_cast<sockaddr *>(&clnt_addr),
                        &clnt_addr_len);
                    // 打印客户端的 ip 地址和端口
                    char ip[50] = {0};
                    printf("检测到有客户端连接: %s, port: %d\n",
                           inet_ntop(AF_INET, &clnt_addr.sin_addr.s_addr, ip,
                                     sizeof(ip)),
                           ntohs(clnt_addr.sin_port));
                    // 将新的通信套接字加入 reads 监听
                    if (clnt_sock > max_fd) {
                        max_fd = clnt_sock;
                    }
                    FD_SET(clnt_sock, &reads);
                }
                // 2. 客户端通信事件
                else {
                    char buf[1024]{};
                    int recv_len = recv(i, buf, sizeof(buf), 0);
                    // 如果接收消息长度为 0 客户端已经断开连接
                    if (recv_len == 0) {
                        printf("客户端断开连接...\n");
                        // NOTE: 客户端断开连接时需要将 fd 从 reads 中删除
                        FD_CLR(i, &reads);
                        close(i);
                    }
                    // 消息长度 > 0 则回传
                    else {
                        printf("接收到客户端发送的消息: %s\n", buf);
                        send(i, buf, recv_len, 0);
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}
