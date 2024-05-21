
#ifndef PACEY_STATUS_RESPONSE_PACKET_HPP
#define PACEY_STATUS_RESPONSE_PACKET_HPP


#include "../packet.hpp"

class StatusResponsePacket : public Packet {
public:
    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        string jsonResponse = R"({
            "version": {
                "name": "1.20.4",
                "protocol": 765
            },
            "players": {
                "max": 100,
                "online": 0,
                "sample": []
            },
            "description": {
                "text": "Bonjour tout le monde !"
            },
            "favicon": "data:image/png;base64,<data>",
            "enforcesSecureChat": false,
            "previewsChat": false
        })";

        writeVarInt(buffer, 0x00);
        writeString(buffer, jsonResponse);

        vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, buffer.size());
        fullPacket.insert(fullPacket.end(), buffer.begin(), buffer.end());

        return fullPacket;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {}
};

#endif
