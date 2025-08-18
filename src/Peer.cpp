#include "Peer.h"
#include <sstream>
#include <algorithm>

Peer::Peer(const std::string& id, const std::string& ip, int p)
    : peer_id(id), ip_address(ip), port(p), is_active(true) {
    updateLastSeen();
}

void Peer::addFile(const FileInfo& file) {
    std::lock_guard<std::mutex> lock(files_mutex);
    
    // Remove if already exists
    auto it = std::find_if(shared_files.begin(), shared_files.end(),
        [&file](const FileInfo& f) { return f.filename == file.filename; });
    
    if (it != shared_files.end()) {
        *it = file;  // Update existing
    } else {
        shared_files.push_back(file);  // Add new
    }
}

void Peer::removeFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(files_mutex);
    shared_files.erase(
        std::remove_if(shared_files.begin(), shared_files.end(),
            [&filename](const FileInfo& f) { return f.filename == filename; }),
        shared_files.end()
    );
}

std::vector<FileInfo> Peer::getFiles() const {
    std::lock_guard<std::mutex> lock(files_mutex);
    return shared_files;
}

bool Peer::hasFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(files_mutex);
    return std::find_if(shared_files.begin(), shared_files.end(),
        [&filename](const FileInfo& f) { return f.filename == filename; }) != shared_files.end();
}

FileInfo Peer::getFileInfo(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(files_mutex);
    auto it = std::find_if(shared_files.begin(), shared_files.end(),
        [&filename](const FileInfo& f) { return f.filename == filename; });
    
    if (it != shared_files.end()) {
        return *it;
    }
    throw std::runtime_error("File not found: " + filename);
}

void Peer::setActive(bool active) {
    is_active.store(active);
    if (active) {
        updateLastSeen();
    }
}

void Peer::updateLastSeen() {
    last_seen = std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point Peer::getLastSeen() const {
    return last_seen;
}

std::string Peer::serialize() const {
    std::ostringstream oss;
    oss << peer_id << "|" << ip_address << "|" << port << "|" << is_active.load();
    
    std::lock_guard<std::mutex> lock(files_mutex);
    oss << "|" << shared_files.size();
    
    for (const auto& file : shared_files) {
        oss << "|" << file.filename << "|" << file.size << "|" << file.hash;
    }
    
    return oss.str();
}

Peer Peer::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 5) {
        throw std::runtime_error("Invalid peer serialization data");
    }
    
    Peer peer(tokens[0], tokens[1], std::stoi(tokens[2]));
    peer.setActive(tokens[3] == "1");
    
    size_t file_count = std::stoul(tokens[4]);
    size_t idx = 5;
    
    for (size_t i = 0; i < file_count && idx + 2 < tokens.size(); ++i) {
        FileInfo file(tokens[idx], "", std::stoul(tokens[idx + 1]), tokens[idx + 2], 0);
        peer.addFile(file);
        idx += 3;
    }
    
    return peer;
}