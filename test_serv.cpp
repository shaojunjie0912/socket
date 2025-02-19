#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr int MAX_EVENTS = 10;  // epoll最大事件数
constexpr int PORT = 9012;
constexpr int BACKLOG = 10;  // 监听队列长度
constexpr int BUFFER_SIZE = 1024;

std::atomic<bool> running(true);  // 服务器运行状态
std::mutex queue_mutex;           // 任务队列互斥锁
std::queue<int> request_queue;    // 任务队列（存储客户端socket）

// 设置socket为非阻塞模式
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 线程池任务：处理客户端请求
void workerThread() {
    while (running) {
        int client_fd = -1;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!request_queue.empty()) {
                client_fd = request_queue.front();
                request_queue.pop();
            }
        }

        if (client_fd > 0) {
            char buffer[BUFFER_SIZE] = {0};
            int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
            if (bytes_read > 0) {
                std::cout << "Received from client " << client_fd << ": " << buffer << std::endl;
                // 回显数据
                send(client_fd, buffer, bytes_read, 0);
            } else if (bytes_read == 0) {
                // 客户端断开连接
                std::cout << "Client " << client_fd << " disconnected.\n";
                close(client_fd);
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                    close(client_fd);
                }
            }
        }
    }
}

// 主服务器逻辑
void serverLoop() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    // 允许端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    // 设置 server_fd 为非阻塞模式
    setNonBlocking(server_fd);

    // 创建 epoll 实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(server_fd);
        return;
    }

    epoll_event event{};
    event.events = EPOLLIN;  // 监听读事件
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        perror("epoll_ctl");
        close(server_fd);
        return;
    }

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back(workerThread);
    }

    epoll_event events[MAX_EVENTS];

    while (running) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count < 0) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < event_count; ++i) {
            int event_fd = events[i].data.fd;

            if (event_fd == server_fd) {
                // 处理新的客户端连接
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                setNonBlocking(client_fd);

                epoll_event client_event{};
                client_event.events = EPOLLIN | EPOLLET;  // 监听可读事件
                client_event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);

                std::cout << "New client connected: " << client_fd << std::endl;
            } else if (events[i].events & EPOLLIN) {
                // 处理客户端可读事件
                std::lock_guard<std::mutex> lock(queue_mutex);
                request_queue.push(event_fd);
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    running = false;

    for (auto &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

int main() {
    std::cout << "Starting server on port " << PORT << "...\n";
    serverLoop();
    return 0;
}
