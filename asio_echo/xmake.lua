add_packages("boost")

-- TCP服务器
target("asio_coro_tcp_echo_server")
    set_kind("binary")
    add_files("tcp/asio_coro_tcp_echo_server.cpp")

-- TCP客户端
target("asio_coro_tcp_echo_client")
    set_kind("binary")
    add_files("tcp/asio_coro_tcp_echo_client.cpp")

-- UDP服务器
target("asio_coro_udp_echo_server")
    set_kind("binary")
    add_files("udp/asio_coro_udp_echo_server.cpp")

-- UDP客户端
target("asio_coro_udp_echo_client")
    set_kind("binary")
    add_files("udp/asio_coro_udp_echo_client.cpp")