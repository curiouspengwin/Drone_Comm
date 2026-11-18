#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct SwarmPacket {
    uint8_t magic = 0xFD;      // Byte 0: Packet identifier
    uint8_t sysid;             // Byte 1: Which drone is this? (1 or 2)
    uint8_t msgType;           // Byte 2: 1 = Telemetry to GCS, 2 = D2D Command
    float posX;              // Bytes 3-6: X position (4-byte float)
    float posY;              // Bytes 7-10: Y position (4-byte float)
    float posZ;              // Bytes 11-14: Z position (4-byte float)
    float battery;           // Bytes 15-18: Battery level (4-byte float)
};
#pragma pack(pop)
