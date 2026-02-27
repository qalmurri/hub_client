#include <asio.hpp>
#include <iostream>
#include "net_client.h"

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cout <<
            "Usage:\n"
            "client HUB_IP HUB_PORT GAME_PORT MY_ID PEER_ID\n";
        return 1;
    }

    asio::io_context io;

    NetClient client(
        io,
        argv[1],
        std::stoi(argv[2]),
        std::stoi(argv[3]),
        argv[4],
        argv[5]
    );

    io.run();
    return 0;
}