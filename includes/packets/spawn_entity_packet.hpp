//
// Created by alpha on 22/05/2024.
//

#ifndef SPAWN_ENTITY_PACKET_HPP
#define SPAWN_ENTITY_PACKET_HPP

#include "../packet.hpp"

class SpawnEntityPacket : public Packet {
public:
    int32_t entity_id;
    std::string entity_uuid;
    int32_t entity_type;
    double x;
    double y;
    double z;
    float pitch;
    float yaw;
    float head_yaw;
    int32_t data;
    int16_t velocity_x;
    int16_t velocity_y;
    int16_t velocity_z;

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> packet;

        // ID du paquet Spawn Entity
        writeVarInt(packet, 0x01);

        // Ajouter les champs du paquet
        writeVarInt(packet, entity_id);
        writeUUID(packet, entity_uuid);
        writeVarInt(packet, entity_type);
        writeDouble(packet, x);
        writeDouble(packet, y);
        writeDouble(packet, z);
        writeAngle(packet, pitch);
        writeAngle(packet, yaw);
        writeAngle(packet, head_yaw);
        writeVarInt(packet, data);
        writeShort(packet, velocity_x);
        writeShort(packet, velocity_y);
        writeShort(packet, velocity_z);

        // Longueur du paquet (données + ID)
        std::vector<uint8_t> fullPacket;
        writeVarInt(fullPacket, packet.size());
        fullPacket.insert(fullPacket.end(), packet.begin(), packet.end());

        return fullPacket;
    }

    void deserialize(const std::vector<uint8_t>& buffer, size_t& position) override {
        entity_id = readVarInt(buffer, position);
        entity_uuid = readUUID(buffer, position);
        entity_type = readVarInt(buffer, position);
        x = readDouble(buffer, position);
        y = readDouble(buffer, position);
        z = readDouble(buffer, position);
        pitch = readAngle(buffer, position);
        yaw = readAngle(buffer, position);
        head_yaw = readAngle(buffer, position);
        data = readVarInt(buffer, position);
        velocity_x = readShort(buffer, position);
        velocity_y = readShort(buffer, position);
        velocity_z = readShort(buffer, position);
    }

};

#endif // SPAWN_ENTITY_PACKET_HPP
