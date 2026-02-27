#pragma once
#include <cstdint>
#include <cstddef>

constexpr size_t ID_SIZE    = 16;
constexpr size_t HEADER_LEN = 1 + ID_SIZE * 2;
constexpr uint8_t PUNCH = 0x06;

enum PacketType : uint8_t {
    DATA_RELAY = 0x02,
    HEARTBEAT  = 0x03,
    PING       = 0x04,
    PONG       = 0x05
};