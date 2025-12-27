#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace asio = boost::asio;

// 定义一个只能传递 int 类型消息的 channel
using IntChannel =
    asio::experimental::channel<asio::any_io_executor, void(boost::system::error_code, int)>;

// 生产者协程
asio::awaitable<void> Producer(std::shared_ptr<IntChannel> ch) {
    try {
        for (int i = 0; i < 10; ++i) {
            std::cout << "Producer: sending " << i << std::endl;
            // 异步发送数据，如果 channel 已满，这里会挂起协程
            co_await ch->async_send(boost::system::error_code{}, i, asio::use_awaitable);

            // 模拟一些生产耗时
            asio::steady_timer timer(co_await asio::this_coro::executor);
            timer.expires_after(std::chrono::milliseconds(100));
            co_await timer.async_wait(asio::use_awaitable);
        }
    } catch (const std::exception& e) {
        std::cerr << "Producer exception: " << e.what() << std::endl;
    }

    std::cout << "Producer: finished sending, closing channel." << std::endl;
    // 发送完成后关闭 channel，这是优雅结束消费者的关键
    ch->close();
}

// 消费者协程
asio::awaitable<void> Consumer(std::shared_ptr<IntChannel> ch) {
    try {
        for (;;) {
            // 异步接收数据，如果 channel 为空，这里会挂起协程
            int received_value = co_await ch->async_receive(asio::use_awaitable);
            std::cout << "Consumer: received " << received_value << std::endl;
        }
    } catch (const boost::system::system_error& e) {
        // 当 channel 被关闭后，async_receive 会立即返回并抛出这个异常
        // 这是正常的、预期的退出方式
        if (e.code() == asio::experimental::error::channel_closed) {
            std::cout << "Consumer: channel closed, exiting." << std::endl;
        } else {
            std::cerr << "Consumer exception: " << e.what() << std::endl;
        }
    }
}

int main() {
    try {
        asio::io_context io_context;

        // 创建一个有缓冲的 channel，缓冲区大小为 3
        // 使用 shared_ptr 是因为需要在多个协程之间共享它
        auto ch = std::make_shared<IntChannel>(io_context, 3);

        // co_spawn 用于启动一个协程
        // asio::detached 表示我们不关心协程的返回值，启动后就任其运行
        asio::co_spawn(io_context, Producer(ch), asio::detached);
        asio::co_spawn(io_context, Consumer(ch), asio::detached);

        // 运行 io_context 事件循环，直到所有异步操作完成
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << std::endl;
    }

    return 0;
}