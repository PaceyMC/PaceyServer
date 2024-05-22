#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <cstring>
#include <random>

#include "../includes/packet.hpp"
#include "../includes/connection_state.hpp"

#include "../includes/packets/login_start_packet.hpp"
#include "../includes/packets/login_success_packet.hpp"
#include "../includes/packets/ping_packet.hpp"
#include "../includes/packets/pong_packet.hpp"
#include "../includes/packets/status_response_packet.hpp"
#include "../includes/packets/plugin_message_packet.hpp"
#include "client_infos_packet.hpp"
#include "keep_alive_packet.hpp"
#include "finish_config.hpp"
#include "spawn_entity_packet.hpp"
#include "bundke.hpp"

bool sendPacket(SOCKET socket, const std::vector<uint8_t>& packet) {
    const int total_bytes = static_cast<int>(packet.size());
    int bytes_sent = 0;

    while (bytes_sent < total_bytes) {
        int result = send(socket, reinterpret_cast<const char*>(&packet[bytes_sent]), total_bytes - bytes_sent, 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "Erreur lors de l'envoi du paquet : " << WSAGetLastError() << std::endl;
            return false;
        }
        bytes_sent += result;
    }

    return true;
}

class Socket {
public:
    Socket() {
#ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != NO_ERROR) {
            throw std::runtime_error("WSAStartup failed: " + std::to_string(iResult));
        }
#endif
        server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create socket");
        }
    }

    ~Socket() {
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#else
        close(server_socket);
#endif
    }

    SOCKET get() const { return server_socket; }

private:
    SOCKET server_socket;
};

// Use the RAII pattern to ensure proper resource cleanup
class ClientHandler {
public:
    ClientHandler(SOCKET client_socket) : client_socket(client_socket), running(true) {
        keep_alive_thread = std::thread(&ClientHandler::keepAliveHandler, this);
    }

    ~ClientHandler() {
        running = false;
        if (keep_alive_thread.joinable()) {
            keep_alive_thread.join();
        }
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }

    void handle() {
        std::vector<uint8_t> received_data;
        size_t buffer_position = 0;

        while (running) {
            char buffer[1024];
            int bytes_received = recv(client_socket, buffer, 1024, 0);
            if (bytes_received <= 0) {
                running = false;
                break;
            }

            received_data.insert(received_data.end(), buffer, buffer + bytes_received);

            try {
                while (buffer_position < received_data.size()) {
                    size_t old_position = buffer_position;
                    int32_t packet_length = Packet::readVarInt(received_data, buffer_position);
                    if (received_data.size() - buffer_position >= packet_length) {
                        int32_t packet_id = Packet::readVarInt(received_data, buffer_position);
                        handlePacket(packet_id, received_data, buffer_position);
                    } else {
                        buffer_position = old_position;
                        break;
                    }
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }

private:
    void keepAliveHandler() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (connection_state == CONFIGURATION) {
                int64_t payload = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                ).count();
                KeepAlivePacket keepAlivePacket(payload);
                sendPacket(client_socket, keepAlivePacket.serialize());
                std::cout << "Keep Alive sent" << std::endl;
            }
        }
    }

    void handlePacket(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        cout << packet_id << endl;
        switch (connection_state) {
            case HANDSHAKE:
                handleHandshake(packet_id, data, pos);
                break;
            case LOGIN:
                handleLogin(packet_id, data, pos);
                break;
            case CONFIGURATION:
                handleConfiguration(packet_id, data, pos);
                break;
            case PLAY:
                handlePlay(packet_id, data, pos);
                break;
            case STATUS:
                handleStatus(packet_id, data, pos);
                break;
        }
    }

    void handleHandshake(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        if (packet_id == 0x00) {
            int32_t protocol_version = Packet::readVarInt(data, pos);
            std::string server_address = Packet::readString(data, pos);
            uint16_t server_port = Packet::readShort(data, pos);
            int32_t next_state = Packet::readVarInt(data, pos);

            std::cout << "Received Handshake: " << std::endl;
            std::cout << "  Protocol Version: " << protocol_version << std::endl;
            std::cout << "  Server Address: " << server_address << std::endl;
            std::cout << "  Server Port: " << server_port << std::endl;
            std::cout << "  Next State: " << next_state << std::endl;

            if (next_state == 1) {
                StatusResponsePacket status_response_packet;
                sendPacket(client_socket, status_response_packet.serialize());
                connection_state = STATUS;
            } else if (next_state == 2) {
                connection_state = LOGIN;
            }
        }
    }

    void handleLogin(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        if (packet_id == 0x00) {
            LoginStartPacket loginStartPacket;
            loginStartPacket.deserialize(data, pos);

            LoginSuccessPacket loginSuccessPacket;
            loginSuccessPacket.player_name = loginStartPacket.player_name;
            loginSuccessPacket.uuid = loginStartPacket.uuid;
            sendPacket(client_socket, loginSuccessPacket.serialize());
        }
        if (packet_id == 0x03) {
            connection_state = CONFIGURATION;
        }
    }

    void handleConfiguration(int32_t packet_id, const std::vector<uint8_t>& data, size_t& buffer_position) {
        if (packet_id == 0x00) {
            ClientInfosPacket clientInfosPacket;
            clientInfosPacket.deserialize(data, buffer_position);
            cout << "Locale : " << clientInfosPacket.locale << endl;
            cout << "View Distance : " << (int)clientInfosPacket.view_distance << endl;
            cout << "Chat Mode : " << clientInfosPacket.chat_mode << endl;
            cout << "Chat Colors : " << clientInfosPacket.chat_colors << endl;
            cout << "Displayed Skin Parts : " << (int)clientInfosPacket.displayed_skin_part << endl;
            cout << "Main Hand : " << clientInfosPacket.main_hand << endl;
            cout << "Enable Text Filtering : " << clientInfosPacket.enable_text_filtering << endl;
            cout << "Allow Server Listing : " << clientInfosPacket.allow_server_listing << endl;

            FinishConfigPacket finishConfigPacket;
            sendPacket(client_socket, finishConfigPacket.serialize());
        }
        else if (packet_id == 0x01) { // Plugin Message
            PluginMessagePacket pluginMessagePacket;
            pluginMessagePacket.deserialize(data, buffer_position);
            if (pluginMessagePacket.channel == "minecraft:brand") {
                std::cout << "Client brand: " << pluginMessagePacket.plugin_message << std::endl;
            } else {
                std::cout << "Plugin Message : " << pluginMessagePacket.plugin_message << std::endl;
            }
        }
        else if (packet_id == 0x02) {
            cout << "MA BITE DE JUIF !!!!!!!!!!!" << endl;
            cout << (connection_state) << endl;

            connection_state = PLAY;
            cout << (connection_state) << endl;
        }
    }

    void handlePlay(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        cout << "MA BITE !!!!!!!!!!!" << endl;
        BundlePacket bundlePacket;
        sendPacket(client_socket, bundlePacket.serialize());
        SpawnEntityPacket packet;
        packet.entity_id = 123;
        packet.entity_uuid = "550e8400-e29b-41d4-a716-446655440000";
        packet.entity_type = 122; // Example entity type
        packet.x = 100.5;
        packet.y = 64.0;
        packet.z = 200.5;
        packet.pitch = 0.0f;
        packet.yaw = 90.0f;
        packet.head_yaw = 90.0f;
        packet.data = 0;
        packet.velocity_x = 0;
        packet.velocity_y = 0;
        packet.velocity_z = 0;
        sendPacket(client_socket, packet.serialize());
        sendPacket(client_socket, bundlePacket.serialize());
    }

    void handleStatus(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        if (packet_id == 0x01) {
            int64_t ping_payload = 0;
            std::memcpy(&ping_payload, &data[pos], sizeof(ping_payload));
            pos += sizeof(ping_payload);
            PongPacket pong_packet(ping_payload);
            sendPacket(client_socket, pong_packet.serialize());

            auto current_time = std::chrono::system_clock::now();
            auto ping_time = std::chrono::milliseconds(ping_payload);
            auto ping_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time.time_since_epoch()) - ping_time;
            std::cout << "Ping: " << ping_duration.count() << " ms" << std::endl;
        }
    }

    SOCKET client_socket;
    ConnectionState connection_state = HANDSHAKE;
    bool running;
    std::thread keep_alive_thread;
};

int main() {
    try {
        Socket server_socket;

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(25565);

        if (bind(server_socket.get(), (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
            throw std::runtime_error("Bind failed");
        }

        if (listen(server_socket.get(), SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed");
        }

        std::cout << "Server listening on port 25565" << std::endl;

        while (true) {
            sockaddr_in client_address;
            int client_address_size = sizeof(client_address);
            SOCKET client_socket = accept(server_socket.get(), (sockaddr*)&client_address, &client_address_size);
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "Client accept failed" << std::endl;
                continue;
            }

            std::cout << "New connection accepted" << std::endl;

            std::thread([](SOCKET socket) {
                ClientHandler handler(socket);
                handler.handle();
            }, client_socket).detach();
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
