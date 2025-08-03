// udp_echo_client_coro.cpp
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>
#include <string>

namespace asio = boost::asio;
using asio::ip::udp;
namespace this_coro = boost::asio::this_coro;

// 定义接收缓冲区大小
constexpr size_t kMaxBufferSize = 1024;

// 客户端主协程
asio::awaitable<void> talker(const std::string& server_ip, uint16_t server_port) {
    // co_await this_coro::executor 用于获取当前协程的执行器
    auto executor = co_await this_coro::executor;
    udp::socket socket(executor, udp::v4());

    udp::endpoint server_endpoint(asio::ip::make_address(server_ip), server_port);

    std::cout << "请输入要发送的消息 (输入 'quit' 退出):" << std::endl;
    std::string message;

    while (std::getline(std::cin, message)) {
        if (message == "quit") {
            break;
        }

        // 1. 异步发送数据
        co_await socket.async_send_to(asio::buffer(message), server_endpoint, asio::use_awaitable);

        // 2. 准备并异步接收回显
        std::vector<char> recv_buffer(kMaxBufferSize);
        udp::endpoint sender_endpoint;
        size_t bytes_received = co_await socket.async_receive_from(
            asio::buffer(recv_buffer), sender_endpoint, asio::use_awaitable);

        std::cout << "服务器回显: " << std::string(recv_buffer.data(), bytes_received) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <服务器IP> <服务器端口>" << std::endl;
        return 1;
    }

    try {
        asio::io_context io_context;

        // 3. 启动 talker 协程
        co_spawn(io_context, talker(argv[1], static_cast<uint16_t>(std::stoi(argv[2]))),
                 // 提供一个完成处理器来捕获协程中可能抛出的异常
                 [](std::exception_ptr e) {
                     if (e) {
                         std::rethrow_exception(e);
                     }
                 });

        // 4. 运行 io_context
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
    }

    return 0;
}