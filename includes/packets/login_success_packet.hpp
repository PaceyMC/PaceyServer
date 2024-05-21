#ifndef PACEY_LOGIN_SUCCESS_PACKET_HPP
#define PACEY_LOGIN_SUCCESS_PACKET_HPP


#include "../packet.hpp"

class LoginSuccessPacket : public Packet {
public:
    string player_name;
    string uuid;

    vector<uint8_t> serialize() const override {
        std::vector<uint8_t> packet;

        // ID du paquet de login success
        writeVarInt(packet, 0x02);

        // Ajout de l'UUID et du nom d'utilisateur
        writeUUID(packet, uuid);
        writeString(packet, player_name);
        writeVarInt(packet, 0);

        // Longueur du paquet (données + ID)
        std::vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, packet.size());
        fullPacket.insert(fullPacket.end(), packet.begin(), packet.end());

        return fullPacket;
    }

    void deserialize(const vector<uint8_t>& buffer, size_t& position) override {}
};

#endif //PACEY_LOGIN_SUCCESS_PACKET_HPP
