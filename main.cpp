#include <iostream>
#include <array>
#include <asio.hpp>

using asio::ip::udp;

constexpr size_t ID_SIZE = 16;
constexpr size_t MAX_PACKET = 2048;

// ===== CONFIG =====
const char* HUB_IP   = "127.0.0.1"; // ganti ke IP VPS
const uint16_t HUB_PORT = 9000;

const uint16_t LOCAL_GAME_PORT = 50000; // game diarahkan ke sini

const char SRC_ID[ID_SIZE] = "CLIENT_A_______";
const char DST_ID[ID_SIZE] = "CLIENT_B_______";
// ==================

enum PacketType : uint8_t {
    DATA_RELAY = 0x02,
    HEARTBEAT  = 0x03,
    PING       = 0x04,
    PONG       = 0x05
};

int main() {
    try {
        asio::io_context io;

        // Socket ke HUB
        udp::socket hub_socket(io);
        hub_socket.open(udp::v4());
        udp::endpoint hub_endpoint(
            asio::ip::make_address(HUB_IP),
            HUB_PORT
        );

        // Socket lokal untuk GAME
        udp::socket game_socket(io, udp::endpoint(udp::v4(), LOCAL_GAME_PORT));

        std::array<char, MAX_PACKET> buffer{};
        udp::endpoint sender;

        std::cout << "[CLIENT] UDP proxy running\n";
        std::cout << "[CLIENT] Game -> localhost:" << LOCAL_GAME_PORT << "\n";

        while (true) {
            // ===== TERIMA DARI GAME =====
            size_t len = game_socket.receive_from(
                asio::buffer(buffer.data() + 33, MAX_PACKET - 33),
                sender
            );

            buffer[0] = DATA_RELAY;
            std::memcpy(buffer.data() + 1,  SRC_ID, ID_SIZE);
            std::memcpy(buffer.data() + 17, DST_ID, ID_SIZE);

            hub_socket.send_to(
                asio::buffer(buffer.data(), len + 33),
                hub_endpoint
            );

            // ===== TERIMA DARI HUB =====
            size_t rlen = hub_socket.receive_from(
                asio::buffer(buffer),
                sender
            );

            if (buffer[0] == DATA_RELAY) {
                game_socket.send_to(
                    asio::buffer(buffer.data() + 33, rlen - 33),
                    udp::endpoint(
                        asio::ip::address_v4::loopback(),
                        LOCAL_GAME_PORT
                    )
                );
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
