cmake_minimum_required(VERSION 3.28)
project(Pacey)

set(CMAKE_CXX_STANDARD 23)

include_directories(includes)
include_directories(includes/packets)

add_executable(Pacey

        includes/packets/handshake_packet.hpp
        includes/packets/login_start_packet.hpp
        includes/packets/login_success_packet.hpp
        includes/packets/ping_packet.hpp
        includes/packets/pong_packet.hpp
        includes/packets/status_response_packet.hpp

        includes/packet.hpp

        sources/main.cpp
)

target_link_libraries(Pacey ws2_32)
