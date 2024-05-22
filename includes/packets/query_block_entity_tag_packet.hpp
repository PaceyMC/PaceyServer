//
// Created by alpha on 22/05/2024.
//

#ifndef PACEY_QUERY_BLOCK_ENTITY_TAG_PACKET_HPP
#define PACEY_QUERY_BLOCK_ENTITY_TAG_PACKET_HPP

#include "packet.hpp"

class QueryBlockPacket : public Packet {
public:
    int32_t transactionID;
    Position blockPos;

    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        return buffer;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
        transactionID = readVarInt(buffer, position);
        blockPos = readPosition(buffer, position);
    }
};


#endif //PACEY_QUERY_BLOCK_ENTITY_TAG_PACKET_HPP
