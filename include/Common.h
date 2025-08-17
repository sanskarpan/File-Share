#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <memory>

// Network includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

// Threading
#include <pthread.h>

// Crypto for hashing
#include <openssl/sha.h>

// Constants
constexpr int DEFAULT_PORT = 8888;
constexpr int BUFFER_SIZE = 8192;
constexpr int MAX_CONNECTIONS = 1024;
constexpr int MAX_EVENTS = 100;

// Protocol message types
enum class MessageType : uint8_t {
    PEER_LIST_REQUEST = 1,
    PEER_LIST_RESPONSE = 2,
    FILE_LIST_REQUEST = 3,
    FILE_LIST_RESPONSE = 4,
    FILE_REQUEST = 5,
    FILE_CHUNK = 6,
    FILE_COMPLETE = 7,
    ERROR_MESSAGE = 8,
    PING = 9,
    PONG = 10
};

// Error codes
enum class ErrorCode : uint8_t {
    SUCCESS = 0,
    FILE_NOT_FOUND = 1,
    PERMISSION_DENIED = 2,
    NETWORK_ERROR = 3,
    PROTOCOL_ERROR = 4
};

#endif