#ifndef PACEY_LOGIN_START_PACKET_HPP
#define PACEY_LOGIN_START_PACKET_HPP

#include "packet.hpp"

class LoginStartPacket : public Packet {
public:
    string player_name;
    string uuid;

    vector<uint8_t> serialize() const override {
        vector<uint8_t> buffer;
        writeString(buffer, player_name);
        return buffer;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {
        player_name = readString(buffer, position);
        uuid = readUUID(buffer, position);
        if (player_name.length() > 16) {
            throw runtime_error("Player name length exceeds the allowed limit");
        }

        cout << "Login Start recieved for the player name : " << player_name << endl;
        cout << "Login Start recieved for the player uuid : " << uuid << endl;
    }
};

#endif
