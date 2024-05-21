#ifndef PACEY_HANDSHAKE_PACKET_HPP
#define PACEY_HANDSHAKE_PACKET_HPP

#include "../packet.hpp"

class HandshakePacket : public Packet {
public:
    int32_t protocol_version;
    string server_address;
    uint16_t server_port;
    int32_t next_state;

    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        writeVarInt(buffer, protocol_version);
        writeString(buffer, server_address);
        buffer.push_back(static_cast<uint8_t>(server_port >> 8));
        buffer.push_back(static_cast<uint8_t>(server_port & 0xFF));
        writeVarInt(buffer, next_state);
        return buffer;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
        protocol_version = readVarInt(buffer, position);
        server_address = readString(buffer, position);
        server_port = readShort(buffer, position);
        next_state = readVarInt(buffer, position);
    }
};

#endif
