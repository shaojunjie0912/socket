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
// #include <mutex>
// #include <thread>

constexpr int kEpollSize = 50;
constexpr int kMaxBufSize = 1024;
constexpr int kBufSize = 2;
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
    if (serv_sock == -1) {
        perror(strerror(errno));
        return -1;
    }
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

    int ep_fd = epoll_create(kEpollSize);

    // 将监听套接字加入监视
    epoll_event event;
    event.data.fd = serv_sock;
    event.events = EPOLLIN;

    epoll_ctl(ep_fd, EPOLL_CTL_ADD, serv_sock, &event);

    epoll_event ep_events[kEpollSize];

    while (true) {
        int event_cnt = epoll_wait(ep_fd, ep_events, kEpollSize, -1);
        if (event_cnt == -1) {
            perror(strerror(errno));
            break;
        }
        for (int i = 0; i < event_cnt; ++i) {
            int fd = ep_events[i].data.fd;
            if (fd == serv_sock) {
                sockaddr_in clnt_addr;
                socklen_t clnt_addr_size;
                int clnt_sock =
                    accept(serv_sock, reinterpret_cast<sockaddr *>(&clnt_addr),
                           &clnt_addr_size);
                SetNonblocking(clnt_sock);
                char ip[50];
                printf("新客户端连接, fd: %d, ip: %s, port: %d\n", clnt_sock,
                       inet_ntop(AF_INET, &clnt_addr.sin_addr.s_addr, ip,
                                 sizeof(ip)),
                       ntohs(clnt_addr.sin_port));
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clnt_sock;
                epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &event);
            } else {
                char buf[kMaxBufSize]{};
                int total_len = 0;
                bool shutdown{false};
                while (true) {
                    // 每次只能读 2, 模拟要接受的数据太多 > 缓冲区大小
                    int cur_len = recv(fd, buf + total_len, kBufSize, 0);
                    if (cur_len == 0) {
                        printf("客户端 fd: %d 断开连接\n", fd);
                        epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        shutdown = true;
                        break; // NOTE: 这里退出循环后不应该再 send
                    } else if (cur_len < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                    } else {
                        total_len += cur_len;
                    }
                }
                if (!shutdown) {
                    send(fd, buf, total_len, 0);
                }
            }
        }
    }
    close(serv_sock); // 关闭监听套接字
    close(ep_fd);     // 关闭 epoll 监视对象文件描述符
    return 0;
}
