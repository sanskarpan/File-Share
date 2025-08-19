#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "Common.h"
#include "Peer.h"

class FileManager {
private:
    std::string shared_directory;
    std::vector<FileInfo> local_files;
    std::mutex files_mutex;
    
    // Helper functions
    std::string calculateFileHash(const std::string& filepath);
    void scanDirectory();
    bool isValidFile(const std::filesystem::path& filepath);

public:
    FileManager();
    ~FileManager() = default;
    
    // Directory management
    void setSharedDirectory(const std::string& directory);
    const std::string& getSharedDirectory() const { return shared_directory; }
    
    // File operations
    void refreshFileList();
    std::vector<FileInfo> getFileList() const;
    bool hasFile(const std::string& filename) const;
    FileInfo getFileInfo(const std::string& filename) const;
    
    // Download management
    bool downloadFile(const std::string& filename, const std::string& peer_address, 
                    int peer_port, const std::string& destination_path);
    
    // Upload management
    void serveFile(int client_socket, const std::string& filename);
    
    // Utility
    bool validateFileIntegrity(const std::string& filepath, const std::string& expected_hash);
    size_t getFileSize(const std::string& filepath);
};

#endif