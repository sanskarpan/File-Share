#ifndef PEERMANAGER_H
#define PEERMANAGER_H

#include "Common.h"
#include "Peer.h"

class PeerManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Peer>> peers;
    mutable std::shared_mutex peers_mutex;
    std::thread heartbeat_thread;
    std::atomic<bool> running;
    
    // Bootstrap and discovery
    std::vector<std::pair<std::string, int>> bootstrap_nodes;
    
    // Heartbeat and maintenance
    void heartbeatLoop();
    void removeStalePeers();
    
    // Network discovery
    void connectToBootstrapNodes();
    void broadcastPeerDiscovery();
    
public:
    PeerManager();
    ~PeerManager();
    
    // Peer management
    void addPeer(std::shared_ptr<Peer> peer);
    void removePeer(const std::string& peer_id);
    std::shared_ptr<Peer> getPeer(const std::string& peer_id) const;
    std::vector<std::shared_ptr<Peer>> getAllPeers() const;
    std::vector<std::shared_ptr<Peer>> getActivePeers() const;
    
    // Bootstrap management
    void addBootstrapNode(const std::string& address, int port);
    void connectToNetwork();
    
    // Search and discovery
    std::vector<std::shared_ptr<Peer>> findPeersWithFile(const std::string& filename) const;
    void updatePeerFileList(const std::string& peer_id, const std::vector<FileInfo>& files);
    
    // Statistics
    size_t getActivePeerCount() const;
    size_t getTotalPeerCount() const;
    
    void start();
    void stop();
};

#endif