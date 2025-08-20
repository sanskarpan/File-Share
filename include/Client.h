#ifndef CLIENT_H
#define CLIENT_H

#include "Common.h"
#include "Peer.h"

struct DownloadProgress {
    std::string filename;
    size_t total_size;
    size_t downloaded_size;
    double speed_mbps;
    std::chrono::steady_clock::time_point start_time;
    std::atomic<bool> completed;
    std::atomic<bool> failed;
    std::string error_message;
};

class Client {
private:
    int socket_fd;
    std::string remote_address;
    int remote_port;
    bool connected;
    
    // Download management
    std::unordered_map<std::string, std::shared_ptr<DownloadProgress>> active_downloads;
    std::mutex downloads_mutex;
    std::thread_pool download_pool;
    
    // Connection management
    bool createSocket();
    void closeSocket();
    
    // Protocol communication
    void sendMessage(MessageType type, const std::vector<uint8_t>& payload);
    std::vector<uint8_t> receiveMessage();
    
    // Multi-source download
    void downloadFromMultipleSources(const std::string& filename, 
                                   const std::vector<std::shared_ptr<Peer>>& sources,
                                   const std::string& destination_path);

public:
    Client();
    ~Client();
    
    // Connection management
    bool connect(const std::string& address, int port);
    void disconnect();
    bool isConnected() const { return connected; }
    
    // Protocol operations
    std::vector<std::string> requestPeerList();
    std::vector<FileInfo> requestFileList(const std::string& peer_id = "");
    bool downloadFile(const std::string& filename, const std::string& destination_path);
    bool downloadFileFromPeer(const std::string& filename, 
                             const std::string& peer_address, 
                             int peer_port,
                             const std::string& destination_path);
    
    // Multi-source downloads
    bool downloadFileMultiSource(const std::string& filename, 
                                const std::vector<std::shared_ptr<Peer>>& sources,
                                const std::string& destination_path);
    
    // Utility
    void sendPing();
    bool sendPong();
    
    // Progress monitoring
    std::shared_ptr<DownloadProgress> getDownloadProgress(const std::string& filename);
    std::vector<std::shared_ptr<DownloadProgress>> getAllDownloads();
    void cancelDownload(const std::string& filename);
};

// Thread pool for concurrent downloads
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    void shutdown();
};

#endif