#include "CLI.h"
#include <csignal>
#include <iostream>

static CLI* g_cli = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully...\n";
    if (g_cli) {
        g_cli->shutdown();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = DEFAULT_PORT;
    std::string share_dir = "./shared/";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = std::atoi(argv[++i]);
            }
        } else if (arg == "-d" || arg == "--directory") {
            if (i + 1 < argc) {
                share_dir = argv[++i];
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  -p, --port PORT       Set listen port (default: " << DEFAULT_PORT << ")\n"
                      << "  -d, --directory DIR   Set shared directory (default: ./shared/)\n"
                      << "  -h, --help           Show this help message\n";
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        CLI cli(port, share_dir);
        g_cli = &cli;
        
        if (!cli.initialize()) {
            std::cerr << "Failed to initialize P2P node\n";
            return 1;
        }
        
        cli.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}