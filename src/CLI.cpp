
#include "CLI.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

CLI::CLI(int port, const std::string& share_dir) 
    : running(false), local_port(port), shared_directory(share_dir) {
    
    server = std::make_unique<HighPerformanceServer>(port);
    client = std::make_unique<Client>();
    peer_manager = std::make_unique<PeerManager>();
    file_manager = std::make_unique<FileManager>();
}

CLI::~CLI() {
    shutdown();
}

bool CLI::initialize() {
    // Set up shared directory
    file_manager->setSharedDirectory(shared_directory);
    server->setSharedDirectory(shared_directory);
    
    // Start server
    if (!server->start()) {
        std::cerr << "Failed to start server on port " << local_port << std::endl;
        return false;
    }
    
    peer_manager->start();
    
    // Add some bootstrap nodes (in real implementation, load from config)
    peer_manager->addBootstrapNode("127.0.0.1", 8889);
    peer_manager->addBootstrapNode("127.0.0.1", 8890);
    
    running = true;
    return true;
}

void CLI::run() {
    displayWelcome();
    
    // Enable readline history
    using_history();
    
    while (running) {
        char* input = readline("p2p> ");
        
        if (!input) {
            // EOF (Ctrl+D)
            std::cout << "\nGoodbye!\n";
            break;
        }
        
        std::string command_line(input);
        free(input);
        
        // Skip empty lines
        if (command_line.empty()) {
            continue;
        }
        
        // Add to history
        add_history(command_line.c_str());
        
        // Parse and execute command
        auto args = parseCommand(command_line);
        if (args.empty()) {
            continue;
        }
        
        std::string command = args[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        try {
            if (command == "peers") {
                handlePeersCommand(args);
            } else if (command == "files") {
                handleFilesCommand(args);
            } else if (command == "get") {
                handleGetCommand(args);
            } else if (command == "share") {
                handleShareCommand(args);
            } else if (command == "connect") {
                handleConnectCommand(args);
            } else if (command == "status") {
                handleStatusCommand(args);
            } else if (command == "downloads") {
                handleDownloadsCommand(args);
            } else if (command == "help") {
                handleHelpCommand(args);
            } else if (command == "exit" || command == "quit") {
                handleExitCommand(args);
            } else {
                std::cout << "Unknown command: " << command << ". Type 'help' for available commands.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

void CLI::shutdown() {
    running = false;
    if (server) {
        server->stop();
    }
    if (peer_manager) {
        peer_manager->stop();
    }
}

std::vector<std::string> CLI::parseCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> std::quoted(token)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void CLI::handlePeersCommand(const std::vector<std::string>& args) {
    auto peers = peer_manager->getAllPeers();
    
    if (peers.empty()) {
        std::cout << "No peers connected.\n";
        return;
    }
    
    std::cout << "Connected Peers (" << peers.size() << "):\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left << std::setw(20) << "Peer ID" 
              << std::setw(20) << "Address" 
              << std::setw(10) << "Status"
              << std::setw(10) << "Files"
              << "Last Seen\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (const auto& peer : peers) {
        auto now = std::chrono::system_clock::now();
        auto diff = now - peer->getLastSeen();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
        
        std::cout << std::left << std::setw(20) << peer->getId().substr(0, 19)
                  << std::setw(20) << peer->getAddress()
                  << std::setw(10) << (peer->isActive() ? "Active" : "Inactive")
                  << std::setw(10) << peer->getFiles().size()
                  << seconds << "s ago\n";
    }
}

void CLI::handleFilesCommand(const std::vector<std::string>& args) {
    if (args.size() > 1 && args[1] == "local") {
        // Show local files
        auto files = file_manager->getFileList();
        std::cout << "Local Files (" << files.size() << "):\n";
        printFileList(files);
    } else if (args.size() > 1) {
        // Show files from specific peer
        auto peer = peer_manager->getPeer(args[1]);
        if (peer) {
            auto files = peer->getFiles();
            std::cout << "Files from peer " << args[1] << " (" << files.size() << "):\n";
            printFileList(files);
        } else {
            std::cout << "Peer not found: " << args[1] << "\n";
        }
    } else {
        // Show all available files from all peers
        auto peers = peer_manager->getActivePeers();
        std::unordered_map<std::string, std::vector<std::string>> file_sources;
        
        for (const auto& peer : peers) {
            auto files = peer->getFiles();
            for (const auto& file : files) {
                file_sources[file.filename].push_back(peer->getId());
            }
        }
        
        if (file_sources.empty()) {
            std::cout << "No files available from peers.\n";
            return;
        }
        
        std::cout << "Available Files (" << file_sources.size() << "):\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << std::left << std::setw(40) << "Filename" 
                  << std::setw(15) << "Sources"
                  << "Peer IDs\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (const auto& [filename, sources] : file_sources) {
            std::cout << std::left << std::setw(40) << filename.substr(0, 39)
                      << std::setw(15) << sources.size();
            
            for (size_t i = 0; i < std::min(sources.size(), size_t(3)); ++i) {
                std::cout << sources[i].substr(0, 8) << " ";
            }
            if (sources.size() > 3) {
                std::cout << "...";
            }
            std::cout << "\n";
        }
    }
}

void CLI::handleGetCommand(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: get <filename> [destination_path]\n";
        return;
    }
    
    std::string filename = args[1];
    std::string destination = (args.size() > 2) ? args[2] : ("./downloads/" + filename);
    
    // Find peers with this file
    auto peers_with_file = peer_manager->findPeersWithFile(filename);
    
    if (peers_with_file.empty()) {
        std::cout << "File not found on any connected peers: " << filename << "\n";
        return;
    }
    
    std::cout << "Found " << peers_with_file.size() << " peer(s) with file: " << filename << "\n";
    
    // Create downloads directory
    std::filesystem::create_directories(std::filesystem::path(destination).parent_path());
    
    // Choose best peer (for now, just use the first active one)
    std::shared_ptr<Peer> chosen_peer = nullptr;
    for (const auto& peer : peers_with_file) {
        if (peer->isActive()) {
            chosen_peer = peer;
            break;
        }
    }
    
    if (!chosen_peer) {
        std::cout << "No active peers found with the file.\n";
        return;
    }
    
    std::cout << "Downloading from peer: " << chosen_peer->getId() 
              << " (" << chosen_peer->getAddress() << ")\n";
    
    // Start download in background thread
    std::thread([this, filename, chosen_peer, destination]() {
        try {
            bool success = client->downloadFileFromPeer(
                filename, 
                chosen_peer->getIpAddress(), 
                chosen_peer->getPort(),
                destination
            );
            
            if (success) {
                std::cout << "\n✓ Download completed: " << filename << "\n";
                std::cout << "p2p> " << std::flush;
            } else {
                std::cout << "\n✗ Download failed: " << filename << "\n";
                std::cout << "p2p> " << std::flush;
            }
        } catch (const std::exception& e) {
            std::cout << "\n✗ Download error: " << e.what() << "\n";
            std::cout << "p2p> " << std::flush;
        }
    }).detach();
}

void CLI::handleShareCommand(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: share <filepath>\n";
        return;
    }
    
    std::string filepath = args[1];
    
    if (!std::filesystem::exists(filepath)) {
        std::cout << "File not found: " << filepath << "\n";
        return;
    }
    
    if (!std::filesystem::is_regular_file(filepath)) {
        std::cout << "Path is not a regular file: " << filepath << "\n";
        return;
    }
    
    // Copy file to shared directory
    std::string filename = std::filesystem::path(filepath).filename();
    std::string dest_path = shared_directory + filename;
    
    try {
        std::filesystem::copy_file(filepath, dest_path, 
                                std::filesystem::copy_options::overwrite_existing);
        
        // Refresh file list
        file_manager->refreshFileList();
        
        std::cout << "File shared successfully: " << filename << "\n";
        std::cout << "Location: " << dest_path << "\n";
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cout << "Failed to share file: " << e.what() << "\n";
    }
}

void CLI::handleStatusCommand(const std::vector<std::string>& args) {
    std::cout << "=== P2P Node Status ===\n";
    std::cout << "Local Port: " << local_port << "\n";
    std::cout << "Shared Directory: " << shared_directory << "\n";
    std::cout << "Active Connections: " << server->getActiveConnectionCount() << "\n";
    std::cout << "Known Peers: " << peer_manager->getTotalPeerCount() << "\n";
    std::cout << "Active Peers: " << peer_manager->getActivePeerCount() << "\n";
    std::cout << "Local Files: " << file_manager->getFileList().size() << "\n";
    
    // Show active downloads
    auto downloads = client->getAllDownloads();
    auto active_downloads = std::count_if(downloads.begin(), downloads.end(),
        [](const auto& dl) { return !dl->completed.load() && !dl->failed.load(); });
    
    std::cout << "Active Downloads: " << active_downloads << "\n";
}

void CLI::handleDownloadsCommand(const std::vector<std::string>& args) {
    auto downloads = client->getAllDownloads();
    
    if (downloads.empty()) {
        std::cout << "No downloads.\n";
        return;
    }
    
    std::cout << "Downloads:\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left << std::setw(30) << "Filename" 
              << std::setw(12) << "Progress"
              << std::setw(15) << "Speed"
              << "Status\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (const auto& dl : downloads) {
        std::string status;
        if (dl->completed.load()) {
            status = "Completed";
        } else if (dl->failed.load()) {
            status = "Failed: " + dl->error_message;
        } else {
            status = "Downloading";
        }
        
        double progress = 0.0;
        if (dl->total_size > 0) {
            progress = (double)dl->downloaded_size / dl->total_size * 100.0;
        }
        
        std::cout << std::left << std::setw(30) << dl->filename.substr(0, 29)
                  << std::setw(12) << (std::to_string((int)progress) + "%")
                  << std::setw(15) << (std::to_string(dl->speed_mbps) + " MB/s")
                  << status << "\n";
    }
}

void CLI::displayWelcome() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════════════════════╗
║                           P2P File Sharing System                             ║
║                                                                               ║
║            A high-performance peer-to-peer file sharing application           ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝

Server started on port: )" << local_port << R"(
Shared directory: )" << shared_directory << R"(

Type 'help' for available commands.

)";
}

void CLI::handleHelpCommand(const std::vector<std::string>& args) {
    std::cout << R"(
Available Commands:
==================

    peers                    - List all connected peers
    files [local|peer_id]   - List files (local, from specific peer, or all)
    get <filename> [dest]   - Download file from peers
    share <filepath>        - Share a file with the network
    connect <ip> <port>     - Connect to a specific peer
    status                  - Show node status and statistics
    downloads               - Show download progress
    help                    - Show this help message
    exit / quit             - Exit the application

Examples:
=========
    files local             - Show your shared files
    files                   - Show all available files from peers
    get example.txt         - Download example.txt to ./downloads/
    get video.mp4 ~/Videos/ - Download video.mp4 to ~/Videos/
    share /home/user/doc.pdf - Share doc.pdf with the network

)";
}

void CLI::handleExitCommand(const std::vector<std::string>& args) {
    std::cout << "Shutting down P2P node...\n";
    running = false;
}

void CLI::printFileList(const std::vector<FileInfo>& files) {
    if (files.empty()) {
        std::cout << "No files available.\n";
        return;
    }
    
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left << std::setw(35) << "Filename" 
              << std::setw(12) << "Size"
              << std::setw(20) << "Hash (first 16)"
              << "Modified\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (const auto& file : files) {
        // Format file size
        std::string size_str;
        if (file.size < 1024) {
            size_str = std::to_string(file.size) + " B";
        } else if (file.size < 1024 * 1024) {
            size_str = std::to_string(file.size / 1024) + " KB";
        } else {
            size_str = std::to_string(file.size / (1024 * 1024)) + " MB";
        }
        
        // Format time
        std::string time_str = std::ctime(&file.last_modified);
        time_str.pop_back(); // Remove newline
        
        std::cout << std::left << std::setw(35) << file.filename.substr(0, 34)
                  << std::setw(12) << size_str
                  << std::setw(20) << file.hash.substr(0, 16)
                  << time_str.substr(4, 12) << "\n";  // Show just date part
    }
}
