#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr int kBuffSize = 1024;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: xmake run udp_echo_client <ip> <port>\n");
        return -1;
    }
    char const *ip = argv[1];
    uint32_t port = atoi(argv[2]);

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);

    int serv_sock = socket(AF_INET, SOCK_DGRAM, 0); // UDP

    sockaddr_in from_addr{};
    char message[kBuffSize]{};

    while (true) {
        scanf("%s", message);
        sendto(serv_sock, message, strlen(message), 0,
               reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr));

        socklen_t from_addr_size = sizeof(from_addr);
        recvfrom(serv_sock, message, kBuffSize, 0,
                 reinterpret_cast<sockaddr *>(&from_addr), &from_addr_size);

        printf("服务器回复：%s\n", message);
    }
    close(serv_sock);
    return 0;
}
