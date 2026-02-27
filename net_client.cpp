#include "net_client.h"
#include "protocol.h"
#include <cstring>
#include <iostream>

using asio::ip::udp;

NetClient::NetClient(
    asio::io_context& io,
    const std::string& hub_ip,
    uint16_t hub_port,
    uint16_t game_port,
    const std::string& my_id,
    const std::string& peer_id
)
    : socket_(io, udp::endpoint(udp::v4(), game_port)),
      hub_(asio::ip::make_address(hub_ip), hub_port),
      game_endpoint_(asio::ip::address_v4::loopback(), game_port)
{
    std::memcpy(src_id_.data(), my_id.data(), ID_SIZE);
    std::memcpy(dst_id_.data(), peer_id.data(), ID_SIZE);

    socket_.set_option(asio::socket_base::receive_buffer_size(1024 * 1024));
    socket_.set_option(asio::socket_base::send_buffer_size(1024 * 1024));

    std::cout << "[CLIENT] Game port : " << game_port << "\n";
    std::cout << "[CLIENT] Hub       : " << hub_ip << ":" << hub_port << "\n";

    send_punch();
    send_ping();

    recv_from_game();
    recv_from_hub();
}

void NetClient::recv_from_game() {
    socket_.async_receive_from(
        asio::buffer(game_buf_), game_sender_,
        [this](std::error_code ec, std::size_t len) {
            if (!ec && len > 0) {
                send_to_hub(len);
            }
            recv_from_game();
        }
    );
}

void NetClient::send_to_hub(std::size_t len) {
    std::memmove(game_buf_.data() + HEADER_LEN, game_buf_.data(), len);

    game_buf_[0] = DATA_RELAY;
    std::memcpy(game_buf_.data() + 1, src_id_.data(), ID_SIZE);
    std::memcpy(game_buf_.data() + 1 + ID_SIZE, dst_id_.data(), ID_SIZE);

    socket_.async_send_to(
        asio::buffer(game_buf_, HEADER_LEN + len),
        hub_,
        [](std::error_code, std::size_t) {}
    );
}

void NetClient::recv_from_hub() {
    socket_.async_receive_from(
        asio::buffer(hub_buf_), hub_sender_,
        [this](std::error_code ec, std::size_t len) {
            if (!ec && len > HEADER_LEN) {
                if (hub_buf_[0] == PONG) {
                    auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(
                        steady_clock::now() - last_ping_
                    ).count();
                    std::cout << "[RTT] " << rtt << " ms\n";
                } else {
                    socket_.async_send_to(
                        asio::buffer(hub_buf_.data() + HEADER_LEN, len - HEADER_LEN),
                        game_endpoint_,
                        [](std::error_code, std::size_t) {}
                    );
                }
            }
            recv_from_hub();
        }
    );
}

void NetClient::send_ping() {
    std::array<char, HEADER_LEN> pkt{};
    pkt[0] = PING;
    std::memcpy(pkt.data() + 1, src_id_.data(), ID_SIZE);
    std::memcpy(pkt.data() + 1 + ID_SIZE, dst_id_.data(), ID_SIZE);

    last_ping_ = steady_clock::now();
    socket_.async_send_to(asio::buffer(pkt), hub_,
        [](std::error_code, std::size_t) {}
    );
}

void NetClient::send_punch() {
    std::array<char, HEADER_LEN> pkt{};
    pkt[0] = PUNCH;
    std::memcpy(pkt.data() + 1, src_id_.data(), ID_SIZE);
    std::memcpy(pkt.data() + 1 + ID_SIZE, dst_id_.data(), ID_SIZE);

    socket_.async_send_to(asio::buffer(pkt), hub_,
        [](std::error_code, std::size_t) {}
    );
}