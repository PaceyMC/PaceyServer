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

#include "../includes/packet.hpp"
#include "../includes/connection_state.hpp"

#include "packets/packet_in/login/login_start_packet.hpp"
#include "packets/packet_out/login/login_success_packet.hpp"
#include "packets/packet_out/status/ping_packet.hpp"
#include "packets/packet_out/status/status_response_packet.hpp"
#include "packets/packet_in/config/plugin_message_packet.hpp"
#include "packets/packet_in/config/client_infos_packet.hpp"
#include "keep_alive_packet.hpp"
#include "packets/packet_out/config/finish_config.hpp"
#include "spawn_entity_packet.hpp"
#include "packet_out/play/EntityAnimPacket.hpp"
#include "utils/functions.hpp"
#include "packet_out/status/pong_packet.hpp"

Logger logger = Packet::getLogger();

bool sendPacket(SOCKET socket, const std::vector<uint8_t>& packet) {
    const int total_bytes = static_cast<int>(packet.size());
    int bytes_sent = 0;

    while (bytes_sent < total_bytes) {
        int result = send(socket, reinterpret_cast<const char*>(&packet[bytes_sent]), total_bytes - bytes_sent, 0);
        if (result == SOCKET_ERROR) {
            logger.error("Erreur lors de l'envoi du paquet : ", WSAGetLastError());
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
            throw runtime_error("WSAStartup failed: " + std::to_string(iResult));
        }
#endif
        server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            throw runtime_error("Failed to create socket");
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
        vector<uint8_t> received_data;
        size_t buffer_position = 0;

        while (running) {
            char buffer[1024];
            int bytes_received = recv(client_socket, buffer, 1024, 0);
            if (bytes_received <= 0) {
                running = false;
                logger.log("FIN !!");
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
                logger.error("Error: ", e.what());
            }
        }
    }

private:
    void keepAliveHandler() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (connection_state == CONFIGURATION) {
                int64_t payload = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                ).count();
                KeepAlivePacket keepAlivePacket(payload);
                sendPacket(client_socket, keepAlivePacket.serialize());
                logger.log("Keep Alive send");
            }
        }
    }

    void handlePacket(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        logger.warn(packet_id);
        switch (connection_state) {
            case HANDSHAKE:
                handleHandshake(packet_id, data, pos);
                break;
            case STATUS:
                handleStatus(packet_id, data, pos);
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
        }
    }

    void handleHandshake(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        if (packet_id == 0x00) {
            int32_t protocol_version = Packet::readVarInt(data, pos);
            std::string server_address = Packet::readString(data, pos);
            uint16_t server_port = Packet::readShort(data, pos);
            int32_t next_state = Packet::readVarInt(data, pos);

            logger.log("Received Handshake:");
            logger.log("  Protocol Version: ", protocol_version);
            logger.log("  Server Address: ", server_address);
            logger.log("  Server Port: ", server_port);
            logger.log("  Next State: ", next_state);

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
            logger.log("CONFIG !!");
            connection_state = CONFIGURATION;
        }
    }

    void handleConfiguration(int32_t packet_id, const std::vector<uint8_t>& data, size_t& buffer_position) {
        if (packet_id == 0x00) {
            ClientInfosPacket clientInfosPacket;
            clientInfosPacket.deserialize(data, buffer_position);
            logger.log("Locale : ", clientInfosPacket.locale);
            logger.log("View Distance : ", static_cast<int>(clientInfosPacket.view_distance));
            logger.log("Chat Mode : ", clientInfosPacket.chat_mode);
            logger.log("Chat Colors : ", clientInfosPacket.chat_colors);
            logger.log("Displayed Skin Parts : ", static_cast<int>(clientInfosPacket.displayed_skin_part));
            logger.log("Main Hand : ", clientInfosPacket.main_hand);
            logger.log("Enable Text Filtering : ", clientInfosPacket.enable_text_filtering);
            logger.log("Allow Server Listing : ", clientInfosPacket.allow_server_listing);
            FinishConfigPacket finishConfigPacket;
            sendPacket(client_socket, finishConfigPacket.serialize());
        }
        else if (packet_id == 0x01) { // Plugin Message
            PluginMessagePacket pluginMessagePacket;
            pluginMessagePacket.deserialize(data, buffer_position);
            if (pluginMessagePacket.channel == "minecraft:brand") {
                logger.log("Client brand : ", pluginMessagePacket.plugin_message);

            } else {
                logger.log("Plugin Message : ", pluginMessagePacket.plugin_message);
            }
        }
        else if (packet_id == 0x02) {
            logger.log("REÇU DE CONFIG FINISH");
            connection_state = PLAY;
        }
        else if (packet_id == 0x03) {
            cout << "PACKET : 0x03" << endl;
        }
    }

    void handlePlay(int32_t packet_id, const std::vector<uint8_t>& data, size_t& pos) {
        logger.log("PLAY !!!");
        EntityAnimPacket entityAnimPacket;
        sendPacket(client_socket, entityAnimPacket.serialize());
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
            logger.log("Ping: ", ping_duration.count(), " ms");
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

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(25565);

        if (bind(server_socket.get(), (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
            throw runtime_error("Bind failed");
        }

        if (listen(server_socket.get(), SOMAXCONN) == SOCKET_ERROR) {
            throw runtime_error("Listen failed");
        }

        logger.log("Server listening on port 25565");

        while (true) {
            sockaddr_in client_address{};
            int client_address_size = sizeof(client_address);
            SOCKET client_socket = accept(server_socket.get(), (sockaddr*)&client_address, &client_address_size);
            if (client_socket == INVALID_SOCKET) {
                logger.error("Client accept failed");
                continue;
            }

            logger.log("New connection accepted : ", inet_ntoa(client_address.sin_addr));

            std::thread([](SOCKET socket) {
                ClientHandler handler(socket);
                handler.handle();
            }, client_socket).detach();
        }
    } catch (const std::runtime_error& e) {
        logger.error("Error : ", e.what());
        return 1;
    }

    return 0;
}
