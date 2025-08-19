#include "FileManager.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

FileManager::FileManager() {
    shared_directory = "./shared/";
    std::filesystem::create_directories(shared_directory);
}

void FileManager::setSharedDirectory(const std::string& directory) {
    shared_directory = directory;
    if (!shared_directory.empty() && shared_directory.back() != '/') {
        shared_directory += '/';
    }
    
    std::filesystem::create_directories(shared_directory);
    refreshFileList();
}

std::string FileManager::calculateFileHash(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for hashing: " + filepath);
    }
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    char buffer[BUFFER_SIZE];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

void FileManager::scanDirectory() {
    std::lock_guard<std::mutex> lock(files_mutex);
    local_files.clear();
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(shared_directory)) {
            if (entry.is_regular_file() && isValidFile(entry.path())) {
                std::string filepath = entry.path().string();
                std::string filename = entry.path().filename().string();
                size_t size = std::filesystem::file_size(entry.path());
                auto last_write = std::filesystem::last_write_time(entry.path());
                
                // Convert to system_clock time_point
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    last_write - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                time_t mod_time = std::chrono::system_clock::to_time_t(sctp);
                
                std::string hash = calculateFileHash(filepath);
                
                local_files.emplace_back(filename, filepath, size, hash, mod_time);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    std::cout << "Scanned " << local_files.size() << " files in " << shared_directory << std::endl;
}

bool FileManager::isValidFile(const std::filesystem::path& filepath) {
    // Skip hidden files and directories
    if (filepath.filename().string().front() == '.') {
        return false;
    }
    
    // Skip system files and common non-shareable extensions
    std::string extension = filepath.extension().string();
    std::vector<std::string> skip_extensions = {".tmp", ".log", ".lock", ".pid"};
    
    return std::find(skip_extensions.begin(), skip_extensions.end(), extension) == skip_extensions.end();
}

void FileManager::refreshFileList() {
    scanDirectory();
}

std::vector<FileInfo> FileManager::getFileList() const {
    std::lock_guard<std::mutex> lock(files_mutex);
    return local_files;
}

bool FileManager::hasFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(files_mutex);
    return std::find_if(local_files.begin(), local_files.end(),
        [&filename](const FileInfo& f) { return f.filename == filename; }) != local_files.end();
}

FileInfo FileManager::getFileInfo(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(files_mutex);
    auto it = std::find_if(local_files.begin(), local_files.end(),
        [&filename](const FileInfo& f) { return f.filename == filename; });
    
    if (it != local_files.end()) {
        return *it;
    }
    
    throw std::runtime_error("File not found: " + filename);
}

bool FileManager::validateFileIntegrity(const std::string& filepath, const std::string& expected_hash) {
    try {
        std::string actual_hash = calculateFileHash(filepath);
        return actual_hash == expected_hash;
    } catch (const std::exception&) {
        return false;
    }
}

size_t FileManager::getFileSize(const std::string& filepath) {
    try {
        return std::filesystem::file_size(filepath);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}