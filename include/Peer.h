#ifndef PEER_H
#define PEER_H

#include "Common.h"

struct FileInfo {
    std::string filename;
    std::string filepath;
    size_t size;
    std::string hash;
    time_t last_modified;
    
    FileInfo(const std::string& name, const std::string& path, 
            size_t sz, const std::string& h, time_t mod)
        : filename(name), filepath(path), size(sz), hash(h), last_modified(mod) {}
};

class Peer {
private:
    std::string peer_id;
    std::string ip_address;
    int port;
    std::vector<FileInfo> shared_files;
    std::mutex files_mutex;
    std::atomic<bool> is_active;
    std::chrono::system_clock::time_point last_seen;

public:
    Peer(const std::string& id, const std::string& ip, int p);
    ~Peer() = default;

    // Getters
    const std::string& getId() const { return peer_id; }
    const std::string& getIpAddress() const { return ip_address; }
    int getPort() const { return port; }
    bool isActive() const { return is_active.load(); }
    
    // File management
    void addFile(const FileInfo& file);
    void removeFile(const std::string& filename);
    std::vector<FileInfo> getFiles() const;
    bool hasFile(const std::string& filename) const;
    FileInfo getFileInfo(const std::string& filename) const;
    
    // Status management
    void setActive(bool active);
    void updateLastSeen();
    std::chrono::system_clock::time_point getLastSeen() const;
    
    // Network address
    std::string getAddress() const { return ip_address + ":" + std::to_string(port); }
    
    // Serialization
    std::string serialize() const;
    static Peer deserialize(const std::string& data);
};

#endif