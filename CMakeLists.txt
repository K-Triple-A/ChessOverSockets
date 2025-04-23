# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(ChessOverSockets)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(client/include)
include_directories(server/include)

# Client executable
add_executable(client
    client/main.cpp
    client/src/client.cpp
    client/src/pieces.cpp
)

# Server executable
add_executable(server
    server/main.cpp
    server/src/network_helper.cpp
)

# Optional: Link libraries if needed (e.g., pthread, etc.)
# target_link_libraries(client pthread)
# target_link_libraries(server pthread)
