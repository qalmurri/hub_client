cmake_minimum_required(VERSION 3.10)

# Nama proyek dan bahasa yang digunakan
project(NetClient VERSION 1.0 LANGUAGES CXX)

# Menggunakan standar C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mencari library threading bawaan OS (dibutuhkan oleh Asio)
find_package(Threads REQUIRED)

# Membuat file executable (asumsi nama file kode C++ kamu adalah main.cpp)
add_executable(client single_file_client.cpp)

# Menggunakan Asio mode standalone (tanpa Boost)
target_compile_definitions(client PRIVATE ASIO_STANDALONE)

# Menautkan library threading
target_link_libraries(client PRIVATE Threads::Threads)

# Jika dikompilasi di OS Windows, Asio butuh library Winsock
if(WIN32)
    target_link_libraries(client PRIVATE ws2_32 wsock32)
endif()

# (OPSIONAL) Jika kamu menaruh folder 'asio/include' di dalam folder proyekmu,
# hapus tanda pagar (#) di bawah ini:
# target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/asio/include)
