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
#include "./utils/logger.hpp"

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
                throw std::runtime_error("Fin du buffer atteinte avant la fin du VarInt");
            }

            currentByte = buffer[position++];
            value |= static_cast<int32_t>(currentByte & SEGMENT_BITS) << shift;

            if ((currentByte & CONTINUE_BIT) == 0) break;

            shift += 7;

            if (shift >= 32) {
                throw std::runtime_error("VarInt trop grand");
            }
        }

        return value;
    }
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
    static int64_t readLong(const std::vector<uint8_t>& buffer, size_t& position) {
        if (position + 8 > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin du long");
        }

        int64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value <<= 8;
            value |= static_cast<uint64_t>(buffer[position++] & 0xFF);
        }
        return value;
    }
    static void writeLong(std::vector<uint8_t>& buffer, int64_t value) {
        for (int i = 7; i >= 0; --i) {
            buffer.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
    }
    static std::vector<uint8_t> readByteArray(const std::vector<uint8_t>& buffer, size_t& position) {
        int32_t array_length = readVarInt(buffer, position);
        if (position + array_length > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin du tableau de bytes");
        }

        std::vector<uint8_t> byteArray(buffer.begin() + position, buffer.begin() + position + array_length);
        position += array_length;
        return byteArray;
    }
    static std::string decimalToHex(int decimal) {
        std::stringstream ss;
        ss << "0x" << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << decimal;
        return ss.str();
    }
    struct Position {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    static Position readPosition(const std::vector<uint8_t>& buffer, size_t& position) {
        if (position + 8 > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin de la position");
        }

        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) | buffer[position++];
        }

        int32_t x = (value >> 38) & 0x3FFFFFF; // Extraire x (26 bits)
        int32_t y = (value >> 26) & 0xFFF;     // Extraire y (12 bits)
        int32_t z = value & 0x3FFFFFF;         // Extraire z (26 bits)

        // Convertir en complément à deux si nécessaire
        if (x >= (1 << 25)) x -= (1 << 26);
        if (y >= (1 << 11)) y -= (1 << 12);
        if (z >= (1 << 25)) z -= (1 << 26);

        return {x, y, z};
    }
    static void writeInt(std::vector<uint8_t>& buffer, int32_t value) {
        buffer.push_back((value >> 24) & 0xFF);
        buffer.push_back((value >> 16) & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
        buffer.push_back(value & 0xFF);
    }
    static void writeBool(std::vector<uint8_t>& buffer, bool value) {
        buffer.push_back(value ? 0x01 : 0x00);
    }
    static void writeDouble(std::vector<uint8_t>& buffer, double value) {
        uint64_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value));
        for (int i = 7; i >= 0; --i) {
            buffer.push_back(static_cast<uint8_t>((raw_value >> (i * 8)) & 0xFF));
        }
    }

    static double readDouble(const std::vector<uint8_t>& buffer, size_t& position) {
        if (position + 8 > buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin du double");
        }
        uint64_t raw_value = 0;
        for (int i = 0; i < 8; ++i) {
            raw_value |= static_cast<uint64_t>(buffer[position++]) << ((7 - i) * 8);
        }
        double value;
        std::memcpy(&value, &raw_value, sizeof(value));
        return value;
    }

    static void writeAngle(std::vector<uint8_t>& buffer, float value) {
        buffer.push_back(static_cast<uint8_t>(value * 256.0f / 360.0f));
    }

    static float readAngle(const std::vector<uint8_t>& buffer, size_t& position) {
        if (position >= buffer.size()) {
            throw std::runtime_error("Fin du buffer atteinte avant la fin de l'angle");
        }
        return static_cast<float>(buffer[position++]) * 360.0f / 256.0f;
    }

    static void writeShort(std::vector<uint8_t>& buffer, int16_t value) {
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    }
    static uint8_t readByte(const std::vector<uint8_t>& data, size_t& index) {
        if (index >= data.size()) {
            throw std::runtime_error("Index out of bounds while reading byte");
        }
        return data[index++];
    }
    static bool readBool(const std::vector<uint8_t>& data, size_t& index) {
        if (index >= data.size()) {
            throw std::runtime_error("Index out of bounds while reading boolean");
        }
        return data[index++] != 0;
    }

    static Logger getLogger() {
        return Logger("Pacey");
    }
};

#endif
