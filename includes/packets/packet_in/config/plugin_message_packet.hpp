//
// Created by alpha on 21/05/2024.
//

#ifndef PACEY_PLUGIN_MESSAGE_PACKET_HPP
#define PACEY_PLUGIN_MESSAGE_PACKET_HPP

#include "packet.hpp"

class PluginMessagePacket : public Packet {
public:
    std::string channel;
    std::string plugin_message;


    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        return buffer;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
        channel = readString(buffer, position);
        plugin_message = readString(buffer, position);
    }
};


#endif //PACEY_PLUGIN_MESSAGE_PACKET_HPP
