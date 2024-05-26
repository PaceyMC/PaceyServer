cmake_minimum_required(VERSION 3.25)
project(Pacey)

set(CMAKE_CXX_STANDARD 23)

include_directories(includes)
include_directories(includes/packets)

add_executable(Pacey

        includes/packets/packet_in/handshake/handshake_packet.hpp
        includes/packets/packet_in/login/login_start_packet.hpp
        includes/packets/packet_out/login/login_success_packet.hpp
        includes/packets/packet_out/status/ping_packet.hpp
        includes/packets/packet_out/status/pong_packet.hpp
        includes/packets/packet_out/status/status_response_packet.hpp

        includes/packet.hpp

        sources/main.cpp
        includes/connection_state.hpp
        includes/packets/keep_alive_packet.hpp
        includes/packets/packet_in/config/plugin_message_packet.hpp
        includes/packets/query_block_entity_tag_packet.hpp
        includes/packets/spawn_entity_packet.hpp
        includes/packets/packet_in/config/client_infos_packet.hpp
        includes/packets/packet_out/config/finish_config.hpp
        includes/packets/packet_out/play/EntityAnimPacket.hpp
        includes/utils/functions.hpp
        includes/utils/logger.hpp
)

target_link_libraries(Pacey ws2_32)
