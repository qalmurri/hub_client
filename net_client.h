#pragma once
#include <asio.hpp>
#include <array>
#include <string>
#include <chrono>

using steady_clock = std::chrono::steady_clock;

class NetClient {
public:
    NetClient(
        asio::io_context& io,
        const std::string& hub_ip,
        uint16_t hub_port,
        uint16_t game_port,
        const std::string& my_id,
        const std::string& peer_id
    );

private:
    void recv_from_game();
    void recv_from_hub();
    void send_to_hub(std::size_t len);
    void send_ping();
    void send_punch();

    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint hub_;
    asio::ip::udp::endpoint game_endpoint_;
    asio::ip::udp::endpoint game_sender_;
    asio::ip::udp::endpoint hub_sender_;

    std::array<char, 8192> game_buf_{};
    std::array<char, 8192> hub_buf_{};

    std::array<char, 16> src_id_{};
    std::array<char, 16> dst_id_{};

    steady_clock::time_point last_ping_;
};