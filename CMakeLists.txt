cmake_minimum_required(VERSION 3.16)
project(P2PFileSharing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/Server.cpp
    src/Client.cpp
    src/Peer.cpp
    src/PeerManager.cpp
    src/FileManager.cpp
    src/Protocol.cpp
)

# Create executable
add_executable(p2p-share ${SOURCES})

# Link libraries
target_link_libraries(p2p-share pthread)

# Enable testing
enable_testing()
find_package(GTest)
if(GTEST_FOUND)
    add_subdirectory(tests)
endif()