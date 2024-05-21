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
#include "../includes/packets/handshake_packet.hpp"
#include "../includes/packets/login_start_packet.hpp"
#include "../includes/packets/login_success_packet.hpp"
#include "../includes/packets/ping_packet.hpp"
#include "../includes/packets/pong_packet.hpp"
#include "../includes/packets/status_response_packet.hpp"
#include "../includes/packets/login_success_packet.hpp"


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

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
#endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(25565);

    if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors du bind" << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#else
        close(server_socket);
#endif
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de l'écoute" << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#else
        close(server_socket);
#endif
        return 1;
    }

    std::cout << "Serveur en écoute sur le port 25565" << std::endl;

    while (true) {
        sockaddr_in client_address;
        int client_address_size = sizeof(client_address);
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Erreur lors de l'acceptation du client" << std::endl;
            continue;
        }

        std::cout << "Nouvelle connexion acceptée" << std::endl;

        std::vector<uint8_t> received_data;
        size_t buffer_position = 0;

        auto last_ping_time = std::chrono::system_clock::now();
        bool encryption_response_sent = false;

        while (true) {
            char buffer[BUFFER_SIZE];
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received == SOCKET_ERROR) {
                std::cerr << "Erreur lors de la réception des données" << std::endl;
                break;
            } else if (bytes_received == 0) {
                std::cout << "Client déconnecté" << std::endl;
                break;
            }

            received_data.insert(received_data.end(), buffer, buffer + bytes_received);

            try {
                while (buffer_position < received_data.size()) {
                    int32_t packet_length = Packet::readVarInt(received_data, buffer_position);
                    if (received_data.size() - buffer_position >= packet_length) {
                        int32_t packet_id = Packet::readVarInt(received_data, buffer_position);
                        if (packet_id == 0x00 && !encryption_response_sent) { // Handshake packet
                            int32_t protocol_version = Packet::readVarInt(received_data, buffer_position);
                            string server_address = Packet::readString(received_data, buffer_position);
                            uint16_t server_port = Packet::readShort(received_data, buffer_position);
                            int32_t next_state = Packet::readVarInt(received_data, buffer_position);

                            std::cout << "Handshake reçu :" << std::endl;
                            std::cout << "  Version de protocole : " << protocol_version << std::endl;
                            std::cout << "  Adresse du serveur : " << server_address << std::endl;
                            std::cout << "  Port du serveur : " << server_port << std::endl;
                            std::cout << "  Prochain état : " << next_state << std::endl;

                            if (next_state == 1) {
                                StatusResponsePacket status_response_packet;
                                sendPacket(client_socket, status_response_packet.serialize());
                            } else if (next_state == 2) {
                                encryption_response_sent = true;
                            }
                        } else if (packet_id == 0x00 && encryption_response_sent) {
                            LoginStartPacket loginStartPacket;
                            loginStartPacket.deserialize(received_data, buffer_position);


                            LoginSuccessPacket loginSuccessPacket;
                            loginSuccessPacket.player_name = loginStartPacket.player_name;
                            loginSuccessPacket.uuid = loginStartPacket.uuid;
                            sendPacket(client_socket, loginSuccessPacket.serialize());
                        }
                        else if (packet_id == 0x01) { // Ping packet
                            int64_t ping_payload = 0;
                            std::memcpy(&ping_payload, &received_data[buffer_position], sizeof(ping_payload));
                            buffer_position += sizeof(ping_payload);
                            PongPacket pong_packet = PongPacket(ping_payload);
                            sendPacket(client_socket, pong_packet.serialize());

                            auto current_time = std::chrono::system_clock::now();
                            auto ping_time = std::chrono::milliseconds(ping_payload);
                            auto ping_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time.time_since_epoch()) - ping_time;
                        }
                        else if (packet_id == 0x03) {
                            cout << "Configuration" << endl;
                        }
                        else {
                            std::cerr << "Paquet inattendu reçu (ID : " << packet_id << ")" << std::endl;
                        }
                    } else {
                        break;
                    }
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "Erreur : " << e.what() << std::endl;
            }

            auto current_time = chrono::system_clock::now();
            if (chrono::duration_cast<chrono::seconds>(current_time - last_ping_time).count() >= 30) {
                int64_t payload = chrono::duration_cast<chrono::milliseconds>(current_time.time_since_epoch()).count();
                PingPacket ping_packet = PingPacket(payload);
                sendPacket(client_socket, ping_packet.serialize());
                last_ping_time = current_time;
            }
        }

#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}
