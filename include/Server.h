#ifndef SERVER_H
#define SERVER_H

#include "Common.h"
#include "PeerManager.h"
#include "FileManager.h"

class Server {
private:
    int server_socket;
    int port;
    std::atomic<bool> running;
    std::thread accept_thread;
    
    // Epoll for non-blocking I/O
    int epoll_fd;
    std::vector<struct epoll_event> events;
    
    // Managers
    std::unique_ptr<PeerManager> peer_manager;
    std::unique_ptr<FileManager> file_manager;
    
    // Connection handling
    void acceptConnections();
    void handleClientConnection(int client_socket);
    void processMessage(int client_socket, const std::vector<uint8_t>& message);
    
    // Message handlers
    void handlePeerListRequest(int client_socket);
    void handleFileListRequest(int client_socket, const std::string& peer_id);
    void handleFileRequest(int client_socket, const std::string& filename);
    
    // Utility
    void sendMessage(int socket, MessageType type, const std::vector<uint8_t>& payload);
    std::vector<uint8_t> receiveMessage(int socket);
    
public:
    Server(int port = DEFAULT_PORT);
    ~Server();
    
    bool start();
    void stop();
    bool isRunning() const { return running.load(); }
    
    // Server management
    void setSharedDirectory(const std::string& directory);
    void addBootstrapPeer(const std::string& address, int port);
};

#endif