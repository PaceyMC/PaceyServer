cmake_minimum_required(VERSION 3.25)
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
        includes/connection_state.hpp
        includes/packets/keep_alive_packet.hpp
        includes/packets/plugin_message_packet.hpp
        includes/packets/query_block_entity_tag_packet.hpp
        includes/packets/spawn_entity_packet.hpp
        includes/packets/client_infos_packet.hpp
        includes/packets/finish_config.hpp
        includes/packets/bundke.hpp
)

target_link_libraries(Pacey ws2_32)
