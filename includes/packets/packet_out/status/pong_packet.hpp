#ifndef PACEY_PONG_PACKET_HPP
#define PACEY_PONG_PACKET_HPP

#include "packet.hpp"

class PongPacket : public Packet {
public:
    int64_t payload;

    PongPacket(int64_t payload) : payload(payload) {}

    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        writeVarInt(buffer, 0x01);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&payload), reinterpret_cast<const uint8_t*>(&payload) + sizeof(payload));

        vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, buffer.size());
        fullPacket.insert(fullPacket.end(), buffer.begin(), buffer.end());

        return fullPacket;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
        if (position + sizeof(payload) > buffer.size()) {
            throw runtime_error("Fin du buffer atteinte avant la fin du pong");
        }
        memcpy(&payload, &buffer[position], sizeof(payload));
        position += sizeof(payload);
    }
};

#endif