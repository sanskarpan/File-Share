#include "HighPerformanceServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <algorithm>

HighPerformanceServer::HighPerformanceServer(int p) : port(p), running(false) {
    peer_manager = std::make_unique<PeerManager>();
    file_manager = std::make_unique<FileManager>();
}

HighPerformanceServer::~HighPerformanceServer() {
    stop();
}

bool HighPerformanceServer::start() {
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create server socket\n";
        return false;
    }
    
    configureSocket(server_socket);
    
    // Bind and listen
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        close(server_socket);
        return false;
    }
    
    if (listen(server_socket, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen on socket\n";
        close(server_socket);
        return false;
    }
    
    // Create epoll
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
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
    event_thread = std::thread(&HighPerformanceServer::eventLoop, this);
    
    peer_manager->start();
    
    std::cout << "High-performance server started on port " << port << std::endl;
    return true;
}

void HighPerformanceServer::stop() {
    if (!running.load()) return;
    
    running.store(false);
    
    if (event_thread.joinable()) {
        event_thread.join();
    }
    
    peer_manager->stop();
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex);
        for (auto& [fd, conn] : connections) {
            close(fd);
        }
        connections.clear();
    }
    
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    
    if (server_socket >= 0) {
        close(server_socket);
    }
    
    std::cout << "High-performance server stopped\n";
}

void HighPerformanceServer::configureSocket(int socket_fd) {
    // Set non-blocking
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Set socket options for performance
    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    
    // Optimize TCP settings
    int tcp_nodelay = 1;
    setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay, sizeof(tcp_nodelay));
    
    // Set send/receive buffer sizes
    int buffer_size = 64 * 1024;  // 64KB
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
}

void HighPerformanceServer::eventLoop() {
    while (running.load()) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, 100);  // 100ms timeout
        
        for (int i = 0; i < event_count; ++i) {
            int fd = events[i].data.fd;
            uint32_t events_mask = events[i].events;
            
            if (fd == server_socket) {
                if (events_mask & EPOLLIN) {
                    handleNewConnection();
                }
            } else {
                if (events_mask & (EPOLLERR | EPOLLHUP)) {
                    closeConnection(fd);
                } else if (events_mask & EPOLLIN) {
                    handleClientData(fd);
                } else if (events_mask & EPOLLOUT) {
                    handleClientWrite(fd);
                }
            }
        }
        
        // Periodic cleanup
        static auto last_cleanup = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup).count() > 60) {
            cleanupStaleConnections();
            last_cleanup = now;
        }
    }
}

void HighPerformanceServer::handleNewConnection() {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // No more connections
            }
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            break;
        }
        
        configureSocket(client_fd);
        
        // Add to epoll
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET;  // Edge-triggered
        event.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
            std::cerr << "Failed to add client to epoll\n";
            close(client_fd);
            continue;
        }
        
        // Create connection object
        std::string peer_addr = inet_ntoa(client_addr.sin_addr);
        auto connection = std::make_unique<Connection>(client_fd, peer_addr);
        
        {
            std::lock_guard<std::mutex> lock(connections_mutex);
            connections[client_fd] = std::move(connection);
        }
        
        std::cout << "New connection from " << peer_addr << " (fd: " << client_fd << ")" << std::endl;
    }
}

size_t HighPerformanceServer::getActiveConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex);
    return connections.size();
}