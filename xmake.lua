set_project("socket")

set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

-- 添加boost依赖
add_requires("boost 1.88.0", { configs = { asio = true } })

includes("simple_socket_echo")
includes("asio_echo")