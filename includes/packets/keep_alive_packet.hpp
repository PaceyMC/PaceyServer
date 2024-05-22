//
// Created by alpha on 21/05/2024.
//

#ifndef PACEY_KEEP_ALIVE_PACKET_HPP
#define PACEY_KEEP_ALIVE_PACKET_HPP

#include <cstdint>
#include "packet.hpp"

class KeepAlivePacket : public Packet {
public:
    int64_t keepAliveID;

    KeepAlivePacket(int64_t id) : keepAliveID(id) {}

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> packet;
        writeVarInt(packet, 0x03); // ID du paquet Keep Alive
        writeLong(packet, keepAliveID);

        std::vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, packet.size());
        fullPacket.insert(fullPacket.end(), packet.begin(), packet.end());

        return fullPacket;
    }

    void deserialize(const std::vector<uint8_t>& buffer, size_t& position) override {
        // Pas besoin de désérialiser pour Keep Alive dans ce contexte
    }
};

#endif //PACEY_KEEP_ALIVE_PACKET_HPP
