#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int kEpollSize = 50;
constexpr int kBuffSize = 2;

int main() {
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9012);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    int ret = bind(serv_sock, reinterpret_cast<sockaddr *>(&serv_addr),
                   sizeof(serv_addr));
    if (ret == -1) {
        printf("bind 失败!\n");
    }
    listen(serv_sock, 10);

    int ep_fd = epoll_create(kEpollSize); // size 没用随便写

    // 将监听套接字加入监视
    epoll_event event;
    event.data.fd = serv_sock;
    event.events = EPOLLIN;

    epoll_ctl(ep_fd, EPOLL_CTL_ADD, serv_sock, &event);

    epoll_event *ep_events = new epoll_event[kEpollSize];

    while (true) {
        int event_cnt = epoll_wait(ep_fd, ep_events, kEpollSize, -1);
        if (event_cnt == -1) {
            printf("epoll_wait 错误!\n");
            break;
        }
        printf("完成 epoll_wait() 调用及事件注册\n");
        // 轮询
        for (int i = 0; i < event_cnt; ++i) {
            // 获取当前事件对应的套接字
            int fd = ep_events[i].data.fd;
            // 如果是连接事件(对应 serv_sock 通信套接字)
            if (fd == serv_sock) {
                sockaddr_in clnt_addr;
                socklen_t clnt_addr_size;
                // 接受客户端连接请求
                int clnt_sock =
                    accept(serv_sock, reinterpret_cast<sockaddr *>(&clnt_addr),
                           &clnt_addr_size);
                // 打印客户端连接信息
                char ip[50];
                printf("新客户端连接, fd: %d, ip: %s, port: %d\n", clnt_sock,
                       inet_ntop(AF_INET, &clnt_addr.sin_addr.s_addr, ip,
                                 sizeof(ip)),
                       ntohs(clnt_addr.sin_port));
                // 将新的通信套接字加入监视
                // NOTE: 默认 LT(Level Trigger)
                // 只要读缓冲区有数据, 就会一直注册事件
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &event);
            }
            // 如果是通信事件
            else {
                char buf[kBuffSize]{};
                int recv_len = recv(fd, buf, kBuffSize, 0);
                if (recv_len == 0) {
                    // 客户端断开连接
                    printf("客户端 fd: %d 断开连接\n", fd);
                    // 移除监视
                    epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd,
                              nullptr); // 最后事件设为空
                    // 关闭通信套接字
                    close(fd);
                } else {
                    printf("获取到客户端消息: %d字节, 并开始回传\n", recv_len);
                    send(fd, buf, recv_len, 0);
                }
            }
        }
    }
    close(serv_sock); // 关闭监听套接字
    close(ep_fd);     // 关闭 epoll 监视对象文件描述符
    return 0;
}
