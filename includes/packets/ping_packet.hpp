//
// Created by alpha on 20/05/2024.
//

#ifndef PACEY_PING_PACKET_HPP
#define PACEY_PING_PACKET_HPP
#include "../packet.hpp"


class PingPacket : public Packet {
public:
    int64_t payload;

    PingPacket(int64_t payload) : payload(payload) {}

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
    }
};


#endif //PACEY_PING_PACKET_HPP
