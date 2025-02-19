// Linux
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

// C
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++
#include <mutex>
#include <thread>

constexpr int kEpollSize = 50;
constexpr int kBuffSize = 1024;
constexpr int kPort = 9012;
constexpr int kBacklog = 10;

int SetNonblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    return flag;
}

int main() {
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(kPort);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    int ret = bind(serv_sock, reinterpret_cast<sockaddr *>(&serv_addr),
                   sizeof(serv_addr));
    if (ret == -1) {
        perror(strerror(errno));
        close(serv_sock);
        return -1;
    }

    listen(serv_sock, kBacklog);

    // HACK: 设置监听套接字非阻塞
    SetNonblocking(serv_sock);

    int ep_fd = epoll_create(kEpollSize); // size 没用随便写

    // 将监听套接字加入监视
    epoll_event event;
    event.data.fd = serv_sock;
    event.events = EPOLLIN;

    epoll_ctl(ep_fd, EPOLL_CTL_ADD, serv_sock, &event);

    epoll_event ep_events[kEpollSize];

    while (true) {
        int event_cnt = epoll_wait(ep_fd, ep_events, kEpollSize, -1);
        if (event_cnt == -1) {
            printf("epoll_wait 错误!\n");
            break;
        }
        printf("完成 epoll_wait() 调用及事件注册\n");
        for (int i = 0; i < event_cnt; ++i) {
            int fd = ep_events[i].data.fd;
            if (fd == serv_sock) {
                sockaddr_in clnt_addr;
                socklen_t clnt_addr_size;
                int clnt_sock =
                    accept(serv_sock, reinterpret_cast<sockaddr *>(&clnt_addr),
                           &clnt_addr_size);
                // HACK: 设置通信套接字非阻塞
                SetNonblocking(clnt_sock);
                char ip[50];
                printf("新客户端连接, fd: %d, ip: %s, port: %d\n", clnt_sock,
                       inet_ntop(AF_INET, &clnt_addr.sin_addr.s_addr, ip,
                                 sizeof(ip)),
                       ntohs(clnt_addr.sin_port));
                // NOTE: ET (Edge Trigger) 只会注册一次事件
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clnt_sock;
                epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &event);
            } else {
                // NOTE: 非阻塞 recv + ET 模式需要循环读数据
                while (true) {
                    char buf[kBuffSize]{};
                    int recv_len = recv(fd, buf, kBuffSize, 0);
                    if (recv_len == 0) {
                        printf("客户端 fd: %d 断开连接\n", fd);
                        // 删除注册, 事件设为空
                        epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        break;
                    }
                    // NOTE: 非阻塞 recv < 0 时需要判断 errno
                    else if (recv_len < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 读缓冲区无数据, 退出读数据循环
                            break;
                        }
                    } else {
                        printf("处理 %d 字节客户端数据\n", recv_len);
                        send(fd, buf, recv_len, 0);
                    }
                }
            }
        }
    }
    close(serv_sock); // 关闭监听套接字
    close(ep_fd);     // 关闭 epoll 监视对象文件描述符
    return 0;
}
