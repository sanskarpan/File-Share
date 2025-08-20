cmake_minimum_required(VERSION 3.16)
project(P2PFileSharing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra -DNDEBUG")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

# Find readline
pkg_check_modules(READLINE REQUIRED readline)

# Include directories
include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR})

# Source files
set(CORE_SOURCES
    src/Peer.cpp
    src/PeerManager.cpp
    src/FileManager.cpp
    src/Server.cpp
    src/Client.cpp
    src/HighPerformanceServer.cpp
    src/Protocol.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
)

set(CLI_SOURCES
    src/CLI.cpp
    src/main.cpp
)

# Create main executable
add_executable(p2p-share ${CORE_SOURCES} ${CLI_SOURCES})

# Link libraries
target_link_libraries(p2p-share 
    Threads::Threads 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${READLINE_LIBRARIES}
)

target_compile_options(p2p-share PRIVATE ${READLINE_CFLAGS_OTHER})

# Create static library for testing
add_library(p2p-core STATIC ${CORE_SOURCES})
target_link_libraries(p2p-core 
    Threads::Threads 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Installation
install(TARGETS p2p-share DESTINATION bin)
install(DIRECTORY shared/ DESTINATION share/p2p-share/ OPTIONAL)

# Create directories for runtime
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shared)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/downloads)

# Package configuration
set(CPACK_PACKAGE_NAME "p2p-file-sharing")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance P2P file sharing application")
set(CPACK_PACKAGE_CONTACT "developer@example.com")
set(CPACK_GENERATOR "TGZ;DEB")

# Debian package specific
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl-dev, libreadline-dev")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")

include(CPack)

# Enable testing
enable_testing()

# Find GTest
find_package(GTest QUIET)
if(GTEST_FOUND)
    add_subdirectory(tests)
    message(STATUS "Tests enabled - GTest found")
else()
    message(STATUS "Tests disabled - GTest not found")
endif()

# Custom targets
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/p2p-share
    DEPENDS p2p-share
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(clean-logs
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.log
)

# Performance testing target
add_custom_target(perf-test
    COMMAND echo "Starting performance test with 200 connections..."
    COMMAND ${CMAKE_BINARY_DIR}/p2p-share --benchmark --connections 200
    DEPENDS p2p-share
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)