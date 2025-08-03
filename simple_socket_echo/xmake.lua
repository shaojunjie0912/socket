target("simple_socket_tcp_echo_server")
    set_kind("binary")
    add_files("tcp/simple_socket_tcp_echo_server.cpp")

target("simple_socket_tcp_echo_client")
    set_kind("binary")
    add_files("tcp/simple_socket_tcp_echo_client.cpp")

target("simple_socket_udp_echo_server")
    set_kind("binary")
    add_files("udp/simple_socket_udp_echo_server.cpp")

target("simple_socket_udp_echo_client")
    set_kind("binary")
    add_files("udp/simple_socket_udp_echo_client.cpp")