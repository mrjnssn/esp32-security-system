// SEC32 Protocol, for documentation see /docs/SEC32-protocol.md in this repo

#pragma once

#include <stdint.h>

// protocol version
constexpr uint8_t PROTOCOL_VERSION = 0x01;

// Node types
constexpr uint8_t NODE_CONTROLLER = 0x01;
constexpr uint8_t NODE_SENSOR_1 = 0x10;

constexpr size_t HEADER_SIZE = 7; 

enum MessageType : uint8_t {
  MSG_SYN     = 0x01,
  MSG_SYN_ACK = 0x02,
  MSG_ACK     = 0x03,
  MSG_FIN     = 0x04,
  MSG_FIN_ACK = 0x05
};

struct PacketHeader {
  uint8_t version;
  uint8_t source;
  uint8_t destination;
  uint8_t type;
  uint16_t sequence;
  uint8_t payloadLength;
};

void serializeHeader(const PacketHeader& header, uint8_t* buffer) {
  buffer[0] = header.version;
  buffer[1] = header.source;
  buffer[2] = header.destination;
  buffer[3] = header.type;
  // because ESP32 is little endian and our protocol is big endian, we need to convert sequence
  buffer[4] = header.sequence >> 8; // right bit shift up 8 places so that high byte becomes the low byte 
  buffer[5] = header.sequence & 0xFF; // bitwise AND
  buffer[6] = header.payloadLength;
};

void deserializeHeader(const uint8_t* buffer, PacketHeader& header) {
  header.version = buffer[0];
  header.source = buffer[1];
  header.destination = buffer[2];
  header.type = buffer[3];

  /* left bit shift for high byte in buffer[4] 
    buffer[4] = 0x12
    buffer[5] = 0x34

    0x12 << 8
    = 0x1200

    then add the low byte from buffer[5] using a bitwise OR
    0x1200 | 0x0034
    = 0x1234 */
  header.sequence = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];
  header.payloadLength = buffer[6];
};