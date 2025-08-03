// tcp_echo_server_coro.cpp
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>
#include <vector>

namespace asio = boost::asio;
using asio::ip::tcp;

// 定义服务器监听的端口
constexpr uint16_t kServerPort = 9090;
// 定义接收缓冲区大小
constexpr size_t kMaxBufferSize = 1024;

// 处理单个客户端连接的协程
asio::awaitable<void> handle_client(tcp::socket socket) {
    try {
        auto remote_endpoint = socket.remote_endpoint();
        std::cout << "新客户端连接: " << remote_endpoint.address().to_string() << ":"
                  << remote_endpoint.port() << std::endl;

        for (;;) {
            // 1. 准备接收数据
            std::vector<char> recv_buffer(kMaxBufferSize);

            // 2. 异步接收数据
            std::cout << "等待客户端 " << remote_endpoint.address().to_string() << ":"
                      << remote_endpoint.port() << " 发送数据..." << std::endl;

            size_t bytes_received =
                co_await socket.async_read_some(asio::buffer(recv_buffer), asio::use_awaitable);

            if (bytes_received == 0) {
                std::cout << "客户端 " << remote_endpoint.address().to_string() << ":"
                          << remote_endpoint.port() << " 断开连接" << std::endl;
                break;
            }

            std::cout << "收到来自 " << remote_endpoint.address().to_string() << ":"
                      << remote_endpoint.port()
                      << " 的消息: " << std::string(recv_buffer.data(), bytes_received)
                      << std::endl;

            // 3. 异步发送数据 (回显)
            co_await socket.async_send(asio::buffer(recv_buffer.data(), bytes_received),
                                       asio::use_awaitable);

            std::cout << "已将消息回显至客户端 " << remote_endpoint.address().to_string() << ":"
                      << remote_endpoint.port() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "处理客户端时发生错误: " << e.what() << std::endl;
    }
}

// 监听协程：接受新连接
asio::awaitable<void> listener(tcp::acceptor& acceptor) {
    for (;;) {
        try {
            // 异步接受新连接
            tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);

            // 为每个新连接启动一个处理协程
            co_spawn(socket.get_executor(), handle_client(std::move(socket)), asio::detached);

        } catch (const std::exception& e) {
            std::cerr << "监听协程中发生错误: " << e.what() << std::endl;
        }
    }
}

int main() {
    try {
        asio::io_context io_context(1);

        // 创建并绑定 acceptor
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), kServerPort));
        std::cout << "TCP服务器已启动, 监听端口 " << kServerPort << "..." << std::endl;

        // 4. 启动 listener 协程
        co_spawn(io_context, listener(acceptor), asio::detached);

        // 优雅地处理程序终止信号 (Ctrl+C)
        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            std::cout << "\n正在关闭服务器..." << std::endl;
            io_context.stop();
        });

        // 5. 运行 io_context
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
    }
    return 0;
}
