//
// Created by alpha on 22/05/2024.
//

#ifndef PACEY_CLIENT_INFOS_PACKET_HPP
#define PACEY_CLIENT_INFOS_PACKET_HPP

#include "packet.hpp"

class ClientInfosPacket : public Packet {
public:
    string locale;
    uint8_t view_distance;
    int32_t chat_mode;
    bool chat_colors;
    uint8_t displayed_skin_part;
    int32_t main_hand;
    bool enable_text_filtering;
    bool allow_server_listing;

    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        return buffer;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
        locale = readString(buffer, position);
        view_distance = readByte(buffer, position);
        chat_mode = readVarInt(buffer, position);
        chat_colors = readBool(buffer, position);
        displayed_skin_part = readByte(buffer, position);
        main_hand = readVarInt(buffer, position);
        enable_text_filtering = readBool(buffer, position);
        allow_server_listing = readBool(buffer, position);

    }
};

#endif //PACEY_CLIENT_INFOS_PACKET_HPP
