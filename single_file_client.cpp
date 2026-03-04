#include <cstdint>
#include <cstddef>
#include <asio.hpp>
#include <array>
#include <string>
#include <chrono>
#include <cstring>
#include <iostream>
#include <algorithm>

using steady_clock = std::chrono::steady_clock;
using asio::ip::udp;

constexpr size_t ID_SIZE    = 16;
constexpr size_t HEADER_LEN = 1 + ID_SIZE * 2;
constexpr uint8_t PUNCH     = 0x06;

enum PacketType : uint8_t {
    DATA_RELAY = 0x02,
    HEARTBEAT  = 0x03,
    PING       = 0x04,
    PONG       = 0x05
};

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
    void start_ping();
    void send_punch();
    void set_id(std::array<char, ID_SIZE>& arr, const std::string& str);

    // Dua socket terpisah untuk mencegah race condition & echo chamber
    asio::ip::udp::socket game_socket_;
    asio::ip::udp::socket hub_socket_;
    asio::steady_timer ping_timer_;

    asio::ip::udp::endpoint hub_endpoint_;
    asio::ip::udp::endpoint game_sender_;
    asio::ip::udp::endpoint hub_sender_;

    std::array<char, 8192> game_buf_{};
    std::array<char, 8192> hub_buf_{};

    std::array<char, ID_SIZE> src_id_{};
    std::array<char, ID_SIZE> dst_id_{};

    steady_clock::time_point last_ping_;
    bool has_game_client_ = false;
};

NetClient::NetClient(
    asio::io_context& io,
    const std::string& hub_ip,
    uint16_t hub_port,
    uint16_t game_port,
    const std::string& my_id,
    const std::string& peer_id
)
    // game_socket_ bind ke localhost untuk mendengarkan game lokal
    : game_socket_(io, udp::endpoint(asio::ip::address_v4::loopback(), game_port)),
      // hub_socket_ menggunakan port acak (0) dari OS untuk komunikasi ke internet
      hub_socket_(io, udp::endpoint(udp::v4(), 0)), 
      ping_timer_(io),
      hub_endpoint_(asio::ip::make_address(hub_ip), hub_port)
{
    set_id(src_id_, my_id);
    set_id(dst_id_, peer_id);

    game_socket_.set_option(asio::socket_base::receive_buffer_size(1024 * 1024));
    hub_socket_.set_option(asio::socket_base::receive_buffer_size(1024 * 1024));

    std::cout << "[CLIENT] Socket listening on 127.0.0.1:" << game_port << "\n";
    std::cout << "[CLIENT] Hub endpoint : " << hub_ip << ":" << hub_port << "\n";

    send_punch();
    start_ping();

    recv_from_game();
    recv_from_hub();
}

// Fungsi aman untuk menyalin ID string ke array char
void NetClient::set_id(std::array<char, ID_SIZE>& arr, const std::string& str) {
    arr.fill(0);
    std::memcpy(arr.data(), str.data(), std::min(str.length(), ID_SIZE));
}

void NetClient::recv_from_game() {
    // Membaca data dengan offset HEADER_LEN agar tidak perlu melakukan memmove
    game_socket_.async_receive_from(
        asio::buffer(game_buf_.data() + HEADER_LEN, game_buf_.size() - HEADER_LEN),
        game_sender_,
        [this](std::error_code ec, std::size_t len) {
            if (!ec && len > 0) {
                has_game_client_ = true; // Menyimpan port asal game lokal

                // Pasang header di depan data
                game_buf_[0] = DATA_RELAY;
                std::memcpy(game_buf_.data() + 1, src_id_.data(), ID_SIZE);
                std::memcpy(game_buf_.data() + 1 + ID_SIZE, dst_id_.data(), ID_SIZE);

                // Kirim lewat hub_socket_
                hub_socket_.async_send_to(
                    asio::buffer(game_buf_.data(), HEADER_LEN + len),
                    hub_endpoint_,
                    [](std::error_code, std::size_t) {}
                );
            }
            recv_from_game();
        }
    );
}

void NetClient::recv_from_hub() {
    hub_socket_.async_receive_from(
        asio::buffer(hub_buf_),
        hub_sender_,
        [this](std::error_code ec, std::size_t len) {
            if (!ec && len > HEADER_LEN) {
                uint8_t type = hub_buf_[0];
                
                if (type == PONG) {
                    auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(
                        steady_clock::now() - last_ping_
                    ).count();
                    std::cout << "[RTT] " << rtt << " ms\n";
                } 
                else if (type == DATA_RELAY && has_game_client_) {
                    // Potong header dan teruskan ke game lokal menggunakan game_socket_
                    game_socket_.async_send_to(
                        asio::buffer(hub_buf_.data() + HEADER_LEN, len - HEADER_LEN),
                        game_sender_,
                        [](std::error_code, std::size_t) {}
                    );
                }
            }
            recv_from_hub();
        }
    );
}

void NetClient::start_ping() {
    std::array<char, HEADER_LEN> pkt{};
    pkt[0] = PING;
    std::memcpy(pkt.data() + 1, src_id_.data(), ID_SIZE);
    std::memcpy(pkt.data() + 1 + ID_SIZE, dst_id_.data(), ID_SIZE);

    last_ping_ = steady_clock::now();
    hub_socket_.async_send_to(
        asio::buffer(pkt), hub_endpoint_,
        [](std::error_code, std::size_t) {}
    );

    // Jadwalkan ping berikutnya 5 detik dari sekarang
    ping_timer_.expires_after(std::chrono::seconds(5));
    ping_timer_.async_wait([this](std::error_code ec) {
        if (!ec) start_ping();
    });
}

void NetClient::send_punch() {
    std::array<char, HEADER_LEN> pkt{};
    pkt[0] = PUNCH;
    std::memcpy(pkt.data() + 1, src_id_.data(), ID_SIZE);
    std::memcpy(pkt.data() + 1 + ID_SIZE, dst_id_.data(), ID_SIZE);

    hub_socket_.async_send_to(
        asio::buffer(pkt), hub_endpoint_,
        [](std::error_code, std::size_t) {}
    );
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cout << "Usage:\n  client HUB_IP HUB_PORT GAME_PORT MY_ID PEER_ID\n";
        return 1;
    }

    try {
        asio::io_context io;
        NetClient client(
            io, argv[1], std::stoi(argv[2]), std::stoi(argv[3]), argv[4], argv[5]
        );
        io.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
