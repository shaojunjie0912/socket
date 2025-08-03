#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

constexpr int kBuffSize = 1024;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: xmake run udp_echo_server <port>\n");
        return -1;
    }
    uint32_t port = atoi(argv[1]);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int serv_sock = socket(AF_INET, SOCK_DGRAM, 0);  // UDP
    int ret = bind(serv_sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
    if (ret == -1) {
        printf("bind error!\n");
        close(serv_sock);
        return -1;
    }

    sockaddr_in clnt_addr{};
    char message[kBuffSize]{};
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while (true) {
        int len = recvfrom(serv_sock, message, kBuffSize, 0,
                           reinterpret_cast<sockaddr*>(&clnt_addr), &clnt_addr_size);
        sendto(serv_sock, message, len, 0, reinterpret_cast<sockaddr*>(&clnt_addr), clnt_addr_size);
    }
    close(serv_sock);
    return 0;
}