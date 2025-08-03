// udp_echo_server_coro.cpp
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <vector>

namespace asio = boost::asio;
using asio::ip::udp;

// 定义服务器监听的端口
constexpr uint16_t kServerPort = 9090;
// 定义接收缓冲区大小
constexpr size_t kMaxBufferSize = 1024;

// 监听协程：该函数是一个异步任务
asio::awaitable<void> listener(udp::socket& socket) {
    for (;;) {
        try {
            // 1. 准备接收数据
            std::vector<char> recv_buffer(kMaxBufferSize);
            udp::endpoint remote_endpoint;

            // 2. 异步接收数据 (使用 co_await)
            //    - co_await 会挂起当前协程，但不会阻塞线程。
            //    - 当数据到达时，io_context 会唤醒此协程，继续执行。
            std::cout << "服务器正在等待数据..." << std::endl;
            size_t bytes_received = co_await socket.async_receive_from(
                asio::buffer(recv_buffer), remote_endpoint, asio::use_awaitable);

            std::cout << "收到来自 " << remote_endpoint.address().to_string() << ":"
                      << remote_endpoint.port()
                      << " 的消息: " << std::string(recv_buffer.data(), bytes_received)
                      << std::endl;

            // 3. 异步发送数据 (回显)
            co_await socket.async_send_to(asio::buffer(recv_buffer.data(), bytes_received),
                                          remote_endpoint, asio::use_awaitable);

            std::cout << "已将消息回显至客户端." << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "监听协程中发生错误: " << e.what() << std::endl;
        }
    }
}

int main() {
    try {
        asio::io_context io_context(1);

        // 创建并绑定 socket
        udp::socket socket(io_context, udp::endpoint(udp::v4(), kServerPort));
        std::cout << "服务器已启动, 监听端口 " << kServerPort << "..." << std::endl;

        // 4. 启动 listener 协程
        //    - co_spawn 在 io_context 上启动一个新的协程。
        //    - asio::detached 表示我们不关心协程的完成情况，让它独立运行。
        co_spawn(io_context, listener(socket), asio::detached);

        // 优雅地处理程序终止信号 (Ctrl+C)
        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        // 5. 运行 io_context
        //    - 此调用会阻塞，并开始处理所有异步事件。
        //    - 当 io_context.stop() 被调用时，run() 会返回。
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
    }
    return 0;
}