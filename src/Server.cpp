#include "Server.h"
#include "Protocol.h"
#include <sys/epoll.h>
#include <algorithm>

Server::Server(int p) : port(p), running(false), epoll_fd(-1) {
    peer_manager = std::make_unique<PeerManager>();
    file_manager = std::make_unique<FileManager>();
    events.resize(MAX_EVENTS);
}

Server::~Server() {
    stop();
}

bool Server::start() {
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket\n";
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options\n";
        close(server_socket);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket to port " << port << "\n";
        close(server_socket);
        return false;
    }
    
    // Listen
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        std::cerr << "Failed to listen on socket\n";
        close(server_socket);
        return false;
    }
    
    // Create epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Failed to create epoll\n";
        close(server_socket);
        return false;
    }
    
    // Add server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) < 0) {
        std::cerr << "Failed to add server socket to epoll\n";
        close(epoll_fd);
        close(server_socket);
        return false;
    }
    
    running.store(true);
    accept_thread = std::thread(&Server::acceptConnections, this);
    
    std::cout << "Server started on port " << port << std::endl;
    return true;
}

void Server::stop() {
    if (!running.load()) return;
    
    running.store(false);
    
    if (accept_thread.joinable()) {
        accept_thread.join();
    }
    
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    
    if (server_socket >= 0) {
        close(server_socket);
    }
    
    std::cout << "Server stopped\n";
}

void Server::acceptConnections() {
    while (running.load()) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, 1000);  // 1 second timeout
        
        for (int i = 0; i < event_count; ++i) {
            if (events[i].data.fd == server_socket) {
                // New connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_socket >= 0) {
                    // Set non-blocking
                    int flags = fcntl(client_socket, F_GETFL, 0);
                    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
                    
                    // Add to epoll
                    struct epoll_event client_event;
                    client_event.events = EPOLLIN | EPOLLET;  // Edge-triggered
                    client_event.data.fd = client_socket;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event);
                    
                    std::cout << "New client connected: " << inet_ntoa(client_addr.sin_addr) 
                              << ":" << ntohs(client_addr.sin_port) << std::endl;
                }
            } else {
                // Handle existing connection
                handleClientConnection(events[i].data.fd);
            }
        }
    }
}

void Server::handleClientConnection(int client_socket) {
    try {
        auto message = receiveMessage(client_socket);
        if (!message.empty()) {
            processMessage(client_socket, message);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
        close(client_socket);
    }
}

void Server::processMessage(int client_socket, const std::vector<uint8_t>& message) {
    if (message.size() < sizeof(MessageType)) {
        return;
    }
    
    MessageType type = static_cast<MessageType>(message[0]);
    std::vector<uint8_t> payload(message.begin() + 1, message.end());
    
    switch (type) {
        case MessageType::PEER_LIST_REQUEST:
            handlePeerListRequest(client_socket);
            break;
            
        case MessageType::FILE_LIST_REQUEST: {
            std::string peer_id(payload.begin(), payload.end());
            handleFileListRequest(client_socket, peer_id);
            break;
        }
        
        case MessageType::FILE_REQUEST: {
            std::string filename(payload.begin(), payload.end());
            handleFileRequest(client_socket, filename);
            break;
        }
        
        default:
            std::cerr << "Unknown message type: " << static_cast<int>(type) << std::endl;
            break;
    }
}

void Server::handlePeerListRequest(int client_socket) {
    auto peers = peer_manager->getAllPeers();
    std::ostringstream oss;
    
    for (const auto& peer : peers) {
        oss << peer.serialize() << "\n";
    }
    
    std::string peer_data = oss.str();
    std::vector<uint8_t> payload(peer_data.begin(), peer_data.end());
    sendMessage(client_socket, MessageType::PEER_LIST_RESPONSE, payload);
}

void Server::handleFileListRequest(int client_socket, const std::string& peer_id) {
    auto files = file_manager->getFileList();
    std::ostringstream oss;
    
    for (const auto& file : files) {
        oss << file.filename << "|" << file.size << "|" << file.hash << "\n";
    }
    
    std::string file_data = oss.str();
    std::vector<uint8_t> payload(file_data.begin(), file_data.end());
    sendMessage(client_socket, MessageType::FILE_LIST_RESPONSE, payload);
}

void Server::handleFileRequest(int client_socket, const std::string& filename) {
    try {
        auto file_info = file_manager->getFileInfo(filename);
        std::ifstream file(file_info.filepath, std::ios::binary);
        
        if (!file.is_open()) {
            std::vector<uint8_t> error_msg(filename.begin(), filename.end());
            sendMessage(client_socket, MessageType::ERROR_MESSAGE, error_msg);
            return;
        }
        
        // Send file in chunks
        std::vector<uint8_t> buffer(BUFFER_SIZE);
        while (file.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE) || file.gcount() > 0) {
            buffer.resize(file.gcount());
            sendMessage(client_socket, MessageType::FILE_CHUNK, buffer);
            buffer.resize(BUFFER_SIZE);
        }
        
        // Send completion signal
        sendMessage(client_socket, MessageType::FILE_COMPLETE, {});
        
    } catch (const std::exception& e) {
        std::vector<uint8_t> error_msg(e.what(), e.what() + strlen(e.what()));
        sendMessage(client_socket, MessageType::ERROR_MESSAGE, error_msg);
    }
}

void Server::sendMessage(int socket, MessageType type, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> message;
    message.push_back(static_cast<uint8_t>(type));
    message.insert(message.end(), payload.begin(), payload.end());
    
    size_t total_sent = 0;
    while (total_sent < message.size()) {
        ssize_t sent = send(socket, message.data() + total_sent, message.size() - total_sent, MSG_NOSIGNAL);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            throw std::runtime_error("Failed to send message");
        }
        total_sent += sent;
    }
}

std::vector<uint8_t> Server::receiveMessage(int socket) {
    std::vector<uint8_t> message;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    
    while (true) {
        ssize_t received = recv(socket, buffer.data(), buffer.size(), MSG_DONTWAIT);
        
        if (received > 0) {
            message.insert(message.end(), buffer.begin(), buffer.begin() + received);
            
            // Check if we have a complete message (simplified - in real implementation, use length prefixes)
            if (received < static_cast<ssize_t>(buffer.size())) {
                break;
            }
        } else if (received == 0) {
            // Connection closed
            throw std::runtime_error("Connection closed by peer");
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // No more data available
            }
            throw std::runtime_error("Failed to receive message");
        }
    }
    
    return message;
}

void Server::setSharedDirectory(const std::string& directory) {
    file_manager->setSharedDirectory(directory);
}

void Server::addBootstrapPeer(const std::string& address, int port) {
    peer_manager->addPeer(std::make_shared<Peer>("bootstrap", address, port));
}