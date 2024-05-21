#ifndef PACEY_PACKET_HPP
#define PACEY_PACKET_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <uuids.h>
#include <rpc.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>

#ifdef _WIN32
#include <winsock2.h>
#include <sstream>
#include <regex>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Rpcrt4.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

using namespace std;

const int BUFFER_SIZE = 1024;
const uint32_t SEGMENT_BITS = 0x7F;
const uint32_t CONTINUE_BIT = 0x80;

class Packet {
public:
    virtual vector<uint8_t> serialize() const = 0;
    virtual void deserialize(const vector<uint8_t>& buffer, size_t& position) = 0;
    virtual ~Packet() = default;

    static int32_t readVarInt(const std::vector<uint8_t>& buffer, size_t& position) {
        int32_t value = 0;
        int shift = 0;
        uint8_t currentByte;

        while (true) {
            if (position >= buffer.size()) {
                throw runtime_error("Fin du buffer atteinte avant la fin du VarInt");
            }

            currentByte = buffer[position++];
            value |= static_cast<int32_t>(currentByte & SEGMENT_BITS) << shift;

            if ((currentByte & CONTINUE_BIT) == 0) break;

            shift += 7;

            if (shift >= 32) throw std::runtime_error("VarInt trop grand");
        }

        return value;
    };
    static void writeVarInt(vector<uint8_t>& buffer, int32_t value) {
        while (true) {
            if ((value & ~SEGMENT_BITS) == 0) {
                buffer.push_back(static_cast<uint8_t>(value));
                return;
            }

            buffer.push_back(static_cast<uint8_t>((value & SEGMENT_BITS) | CONTINUE_BIT));
            value >>= 7;
        }
    };
    static string readString(const vector<uint8_t>& buffer, size_t& position) {
        int32_t string_length = readVarInt(buffer, position);
        if (string_length > 64) {
            throw std::runtime_error("String length exceeds the allowed limit");
        }
        if (position + string_length > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin de la chaîne");
        }

        std::string result(buffer.begin() + position, buffer.begin() + position + string_length);
        position += string_length;
        return result;
    };
    static void writeString(vector<uint8_t>& buffer, const string& value) {
        writeVarInt(buffer, value.length());

        for (char c : value) {
            buffer.push_back(static_cast<uint8_t>(c));
        }
    };
    static uint16_t readShort(const vector<uint8_t>& buffer, size_t& position) {
        if (position + 2 > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin du short");
        }

        uint16_t result = static_cast<uint16_t>(buffer[position]) << 8 |
                          static_cast<uint16_t>(buffer[position + 1]);
        position += 2;
        return result;
    };

    static std::string generate_uuid(size_t len) {
        static const char x[] = "0123456789abcdef";
        std::string uuid;
        uuid.reserve(len);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(x) - 2);
        for (size_t i = 0; i < len; i++)
            uuid += x[dis(gen)];
        return uuid;
    }

    static string readUUID(const std::vector<uint8_t>& buffer, size_t& position) {
        if (position + 16 > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin de l'UUID");
        }

        std::stringstream uuid;
        uuid << std::hex << std::setfill('0');

        for (int i = 0; i < 16; ++i) {
            uint8_t byte = buffer[position++];
            uuid << std::setw(2) << static_cast<int>(byte);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                uuid << "-";
            }
        }

        return uuid.str();
    }
    static void writeUUID(std::vector<uint8_t>& buffer, const std::string& uuid) {
        if (uuid.size() != 36 || uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-') {
            throw std::invalid_argument("UUID invalide");
        }

        for (size_t i = 0; i < uuid.size(); ++i) {
            if (uuid[i] == '-') continue;
            uint8_t byte = std::stoi(uuid.substr(i, 2), nullptr, 16);
            buffer.push_back(byte);
            ++i;
        }
    }
    static void writeArray(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& array) {
        writeVarInt(buffer, static_cast<int32_t>(array.size()));
        buffer.insert(buffer.end(), array.begin(), array.end());
    }
};

#endif
