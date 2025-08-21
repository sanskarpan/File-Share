#ifndef HIGH_PERFORMANCE_SERVER_H
#define HIGH_PERFORMANCE_SERVER_H

#include "Common.h"
#include "PeerManager.h"
#include "FileManager.h"
#include <sys/epoll.h>
#include <unordered_map>

struct Connection {
    int socket_fd;
    std::string peer_address;
    std::vector<uint8_t> read_buffer;
    std::vector<uint8_t> write_buffer;
    size_t bytes_read;
    size_t bytes_written;
    std::chrono::steady_clock::time_point last_activity;
    enum State { READING_HEADER, READING_BODY, WRITING_RESPONSE } state;
    uint32_t expected_message_size;
    
    Connection(int fd, const std::string& addr) 
        : socket_fd(fd), peer_address(addr), bytes_read(0), bytes_written(0),
          last_activity(std::chrono::steady_clock::now()), 
          state(READING_HEADER), expected_message_size(0) {
        read_buffer.resize(BUFFER_SIZE);
    }
};

class HighPerformanceServer {
private:
    int server_socket;
    int epoll_fd;
    int port;
    std::atomic<bool> running;
    
    // Connection management
    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    std::mutex connections_mutex;
    
    // Event processing
    std::array<epoll_event, MAX_EVENTS> events;
    std::thread event_thread;
    
    // Managers
    std::unique_ptr<PeerManager> peer_manager;
    std::unique_ptr<FileManager> file_manager;
    
    // Core operations
    void eventLoop();
    void handleNewConnection();
    void handleClientData(int client_fd);
    void handleClientWrite(int client_fd);
    void closeConnection(int client_fd);
    
    // Message processing
    void processCompleteMessage(Connection* conn, const std::vector<uint8_t>& message);
    void queueResponse(Connection* conn, MessageType type, const std::vector<uint8_t>& payload);
    
    // Optimization
    void configureSocket(int socket_fd);
    void cleanupStaleConnections();
    
public:
    HighPerformanceServer(int port = DEFAULT_PORT);
    ~HighPerformanceServer();
    
    bool start();
    void stop();
    
    // Statistics
    size_t getActiveConnectionCount() const;
    double getAverageResponseTime() const;
    size_t getBytesTransferred() const;
};

#endif