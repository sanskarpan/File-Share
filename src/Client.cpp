#include "Client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <future>

Client::Client() : socket_fd(-1), remote_port(0), connected(false) {}

Client::~Client() {
    disconnect();
}

bool Client::createSocket() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 seconds
    timeout.tv_usec = 0;
    
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    return true;
}

void Client::closeSocket() {
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
    connected = false;
}

bool Client::connect(const std::string& address, int port) {
    if (connected) {
        disconnect();
    }
    
    if (!createSocket()) {
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << address << std::endl;
        closeSocket();
        return false;
    }
    
    if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to " << address << ":" << port << std::endl;
        closeSocket();
        return false;
    }
    
    remote_address = address;
    remote_port = port;
    connected = true;
    
    return true;
}

void Client::disconnect() {
    closeSocket();
    remote_address.clear();
    remote_port = 0;
}

void Client::sendMessage(MessageType type, const std::vector<uint8_t>& payload) {
    if (!connected) {
        throw std::runtime_error("Not connected to any peer");
    }
    
    // Create message with type prefix
    std::vector<uint8_t> message;
    message.push_back(static_cast<uint8_t>(type));
    message.insert(message.end(), payload.begin(), payload.end());
    
    // Send message length first
    uint32_t length = htonl(message.size());
    if (send(socket_fd, &length, sizeof(length), 0) != sizeof(length)) {
        throw std::runtime_error("Failed to send message length");
    }
    
    // Send actual message
    size_t total_sent = 0;
    while (total_sent < message.size()) {
        ssize_t sent = send(socket_fd, message.data() + total_sent, 
                           message.size() - total_sent, 0);
        if (sent <= 0) {
            throw std::runtime_error("Failed to send message data");
        }
        total_sent += sent;
    }
}

std::vector<uint8_t> Client::receiveMessage() {
    if (!connected) {
        throw std::runtime_error("Not connected to any peer");
    }
    
    // Receive message length
    uint32_t length;
    if (recv(socket_fd, &length, sizeof(length), MSG_WAITALL) != sizeof(length)) {
        throw std::runtime_error("Failed to receive message length");
    }
    length = ntohl(length);
    
    if (length > 10 * 1024 * 1024) {  // 10MB max message size
        throw std::runtime_error("Message too large");
    }
    
    // Receive actual message
    std::vector<uint8_t> message(length);
    size_t total_received = 0;
    
    while (total_received < length) {
        ssize_t received = recv(socket_fd, message.data() + total_received, 
                               length - total_received, 0);
        if (received <= 0) {
            throw std::runtime_error("Failed to receive message data");
        }
        total_received += received;
    }
    
    return message;
}

std::vector<std::string> Client::requestPeerList() {
    sendMessage(MessageType::PEER_LIST_REQUEST, {});
    auto response = receiveMessage();
    
    if (response.empty() || response[0] != static_cast<uint8_t>(MessageType::PEER_LIST_RESPONSE)) {
        throw std::runtime_error("Invalid peer list response");
    }
    
    std::string data(response.begin() + 1, response.end());
    std::vector<std::string> peers;
    std::istringstream iss(data);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            peers.push_back(line);
        }
    }
    
    return peers;
}

std::vector<FileInfo> Client::requestFileList(const std::string& peer_id) {
    std::vector<uint8_t> payload(peer_id.begin(), peer_id.end());
    sendMessage(MessageType::FILE_LIST_REQUEST, payload);
    
    auto response = receiveMessage();
    if (response.empty() || response[0] != static_cast<uint8_t>(MessageType::FILE_LIST_RESPONSE)) {
        throw std::runtime_error("Invalid file list response");
    }
    
    std::string data(response.begin() + 1, response.end());
    std::vector<FileInfo> files;
    std::istringstream iss(data);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            std::istringstream line_stream(line);
            std::string filename, size_str, hash;
            
            if (std::getline(line_stream, filename, '|') &&
                std::getline(line_stream, size_str, '|') &&
                std::getline(line_stream, hash)) {
                
                try {
                    size_t size = std::stoull(size_str);
                    files.emplace_back(filename, "", size, hash, 0);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to parse file info: " << e.what() << std::endl;
                }
            }
        }
    }
    
    return files;
}

bool Client::downloadFile(const std::string& filename, const std::string& destination_path) {
    // Create progress tracker
    auto progress = std::make_shared<DownloadProgress>();
    progress->filename = filename;
    progress->downloaded_size = 0;
    progress->completed.store(false);
    progress->failed.store(false);
    progress->start_time = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(downloads_mutex);
        active_downloads[filename] = progress;
    }
    
    try {
        // Request file
        std::vector<uint8_t> payload(filename.begin(), filename.end());
        sendMessage(MessageType::FILE_REQUEST, payload);
        
        // Open destination file
        std::ofstream output_file(destination_path, std::ios::binary | std::ios::trunc);
        if (!output_file.is_open()) {
            throw std::runtime_error("Cannot create destination file: " + destination_path);
        }
        
        size_t total_downloaded = 0;
        auto last_update = std::chrono::steady_clock::now();
        
        while (true) {
            auto response = receiveMessage();
            if (response.empty()) {
                break;
            }
            
            MessageType msg_type = static_cast<MessageType>(response[0]);
            std::vector<uint8_t> data(response.begin() + 1, response.end());
            
            switch (msg_type) {
                case MessageType::FILE_CHUNK: {
                    output_file.write(reinterpret_cast<const char*>(data.data()), data.size());
                    total_downloaded += data.size();
                    progress->downloaded_size = total_downloaded;
                    
                    // Update speed calculation
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
                    if (elapsed.count() > 1000) {  // Update every second
                        auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - progress->start_time);
                        if (total_elapsed.count() > 0) {
                            double mbps = (total_downloaded / 1024.0 / 1024.0) / total_elapsed.count();
                            progress->speed_mbps = mbps;
                        }
                        last_update = now;
                    }
                    break;
                }
                
                case MessageType::FILE_COMPLETE: {
                    progress->completed.store(true);
                    progress->total_size = total_downloaded;
                    std::cout << "Download completed: " << filename << " (" << total_downloaded << " bytes)" << std::endl;
                    return true;
                }
                
                case MessageType::ERROR_MESSAGE: {
                    std::string error_msg(data.begin(), data.end());
                    progress->failed.store(true);
                    progress->error_message = error_msg;
                    throw std::runtime_error("Server error: " + error_msg);
                }
                
                default:
                    std::cerr << "Unexpected message type during download: " << static_cast<int>(msg_type) << std::endl;
                    break;
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        progress->failed.store(true);
        progress->error_message = e.what();
        std::cerr << "Download failed: " << e.what() << std::endl;
        return false;
    }
}

bool Client::downloadFileFromPeer(const std::string& filename, 
                                 const std::string& peer_address, 
                                 int peer_port,
                                 const std::string& destination_path) {
    Client peer_client;
    if (!peer_client.connect(peer_address, peer_port)) {
        return false;
    }
    
    bool result = peer_client.downloadFile(filename, destination_path);
    peer_client.disconnect();
    return result;
}

std::shared_ptr<DownloadProgress> Client::getDownloadProgress(const std::string& filename) {
    std::lock_guard<std::mutex> lock(downloads_mutex);
    auto it = active_downloads.find(filename);
    return (it != active_downloads.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<DownloadProgress>> Client::getAllDownloads() {
    std::lock_guard<std::mutex> lock(downloads_mutex);
    std::vector<std::shared_ptr<DownloadProgress>> result;
    
    for (const auto& pair : active_downloads) {
        result.push_back(pair.second);
    }
    
    return result;
}

void Client::sendPing() {
    sendMessage(MessageType::PING, {});
}

bool Client::sendPong() {
    try {
        sendMessage(MessageType::PONG, {});
        return true;
    } catch (const std::exception&) {
        return false;
    }
}