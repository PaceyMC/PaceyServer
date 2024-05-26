//
// Created by alpha on 26/05/2024.
//

#ifndef PACEY_ENTITYANIMPACKET_HPP
#define PACEY_ENTITYANIMPACKET_HPP
#include "packet.hpp"

class EntityAnimPacket : public Packet {
public:

    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        writeVarInt(buffer, 0x03);
        writeVarInt(buffer, 1);

        vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, buffer.size());
        fullPacket.insert(fullPacket.end(), buffer.begin(), buffer.end());

        return fullPacket;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
    }
};

#endif //PACEY_ENTITYANIMPACKET_HPP
