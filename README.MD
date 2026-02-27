# MinGW
## g++ -std=c++17 main.cpp net_client.cpp -lws2_32 -o lan_client.exe
# MSVC
## cl /std:c++17 main.cpp net_client.cpp ws2_32.lib

HUB (Linux): ./hub 9000
Client PC A: client.exe 1.2.3.4 9000 7777 PLAYER_A PLAYER_B
Client PC B: client.exe 1.2.3.4 9000 7777 PLAYER_B PLAYER_A
Di game (KEDUA PC): Direct IP / LAN 127.0.0.1