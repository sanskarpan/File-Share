#ifndef CLI_H
#define CLI_H

#include "Common.h"
#include "HighPerformanceServer.h"
#include "Client.h"
#include "PeerManager.h"
#include "FileManager.h"
#include <readline/readline.h>
#include <readline/history.h>

class CLI {
private:
    std::unique_ptr<HighPerformanceServer> server;
    std::unique_ptr<Client> client;
    std::unique_ptr<PeerManager> peer_manager;
    std::unique_ptr<FileManager> file_manager;
    
    bool running;
    int local_port;
    std::string shared_directory;
    
    // Command handlers
    void handlePeersCommand(const std::vector<std::string>& args);
    void handleFilesCommand(const std::vector<std::string>& args);
    void handleGetCommand(const std::vector<std::string>& args);
    void handleShareCommand(const std::vector<std::string>& args);
    void handleConnectCommand(const std::vector<std::string>& args);
    void handleStatusCommand(const std::vector<std::string>& args);
    void handleDownloadsCommand(const std::vector<std::string>& args);
    void handleHelpCommand(const std::vector<std::string>& args);
    void handleExitCommand(const std::vector<std::string>& args);
    
    // Utilities
    std::vector<std::string> parseCommand(const std::string& input);
    void printPeerList(const std::vector<std::shared_ptr<Peer>>& peers);
    void printFileList(const std::vector<FileInfo>& files);
    void printDownloadProgress();
    void displayWelcome();
    void displayPrompt();

public:
    CLI(int port = DEFAULT_PORT, const std::string& share_dir = "./shared/");
    ~CLI();
    
    bool initialize();
    void run();
    void shutdown();
};

#endif
