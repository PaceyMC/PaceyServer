//
// Created by alpha on 23/05/2024.
//

#ifndef PACEY_FINISH_CONFIG_HPP
#define PACEY_FINISH_CONFIG_HPP

#include "packet.hpp"

class FinishConfigPacket : public Packet {
public:


    vector<uint8_t> serialize() const override {
        std::vector<uint8_t> packet;

        // ID du paquet de login success
        writeVarInt(packet, 0x02);

        // Longueur du paquet (données + ID)
        std::vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, packet.size());
        fullPacket.insert(fullPacket.end(), packet.begin(), packet.end());

        return fullPacket;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {}
};

#endif //PACEY_FINISH_CONFIG_HPP
